/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>
#include <array>
#include <cstring>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include "xenia/kernel/xboxkrnl/xecrypt_sha.h"

#include "third_party/catch/include/catch.hpp"

namespace {

using xe::kernel::xboxkrnl::XECRYPT_SHA_STATE;
using xe::kernel::xboxkrnl::XeCryptSha;
using xe::kernel::xboxkrnl::XeCryptShaFinal;
using xe::kernel::xboxkrnl::XeCryptShaInit;
using xe::kernel::xboxkrnl::XeCryptShaUpdate;

using Digest = std::array<uint8_t, 20>;

Digest FromHex(std::string_view hex) {
  Digest digest{};
  for (size_t i = 0; i < digest.size(); i++) {
    digest[i] = static_cast<uint8_t>(
        std::stoi(std::string(hex.substr(i * 2, 2)), nullptr, 16));
  }
  return digest;
}

const uint8_t* Bytes(std::string_view text) {
  return reinterpret_cast<const uint8_t*>(text.data());
}

Digest Sha(std::string_view message) {
  Digest digest;
  XeCryptSha(Bytes(message), static_cast<uint32_t>(message.size()), nullptr, 0,
             nullptr, 0, digest.data(), static_cast<uint32_t>(digest.size()));
  return digest;
}

Digest Final(XECRYPT_SHA_STATE& state) {
  Digest digest;
  XeCryptShaFinal(&state, digest.data(), static_cast<uint32_t>(digest.size()));
  return digest;
}

}  // namespace

TEST_CASE("XeCryptSha_known_answers", "[crypt]") {
  REQUIRE(Sha("") == FromHex("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
  REQUIRE(Sha("abc") == FromHex("a9993e364706816aba3e25717850c26c9cd0d89d"));
  REQUIRE(Sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          FromHex("84983e441c3bd26ebaae4aa1f95129e5e54670f1"));
}

TEST_CASE("XeCryptSha_joins_inputs_and_truncates_output", "[crypt]") {
  const std::string_view inputs[] = {
      "abcdbcdecdefdefgefgh", "fghighijhijkijkljklmklmn", "lmnomnopnopq"};
  std::array<uint8_t, 24> out;
  out.fill(0xCC);
  XeCryptSha(Bytes(inputs[0]), static_cast<uint32_t>(inputs[0].size()),
             Bytes(inputs[1]), static_cast<uint32_t>(inputs[1].size()),
             Bytes(inputs[2]), static_cast<uint32_t>(inputs[2].size()),
             out.data(), 8);
  const Digest expected = FromHex("84983e441c3bd26ebaae4aa1f95129e5e54670f1");
  REQUIRE(std::equal(out.begin(), out.begin() + 8, expected.begin()));
  REQUIRE(std::all_of(out.begin() + 8, out.end(),
                      [](uint8_t b) { return b == 0xCC; }));
}

TEST_CASE("XeCryptShaUpdate_streams_across_blocks", "[crypt]") {
  // Uneven chunks start and end at every offset within a block.
  const std::vector<uint8_t> message(1000000, 'a');
  XECRYPT_SHA_STATE state;
  XeCryptShaInit(&state);
  size_t offset = 0;
  size_t chunk = 1;
  while (offset < message.size()) {
    const size_t size = std::min(chunk, message.size() - offset);
    XeCryptShaUpdate(&state, message.data() + offset,
                     static_cast<uint32_t>(size));
    offset += size;
    chunk = chunk * 3 % 1000 + 1;
  }
  REQUIRE(state.count == 1000000);
  REQUIRE(Final(state) == FromHex("34aa973cd4c4daa4f61eeb2bdbad27316534016f"));
}

TEST_CASE("XeCryptShaFinal_keeps_count_and_last_block", "[crypt]") {
  XECRYPT_SHA_STATE state;
  XeCryptShaInit(&state);
  XeCryptShaUpdate(&state, Bytes("abc"), 3);
  const Digest digest = Final(state);

  REQUIRE(state.count == 3);
  REQUIRE(std::memcmp(state.state, digest.data(), digest.size()) == 0);
  std::array<uint8_t, 64> block{'a', 'b', 'c', 0x80};
  block[63] = 0x18;
  REQUIRE(std::memcmp(state.buffer, block.data(), block.size()) == 0);
}

TEST_CASE("XeCryptShaFinal_twice_after_two_padding_blocks", "[crypt]") {
  // 60 bytes need a second padding block, which overwrites the message tail
  // the second Final pads again.
  std::array<uint8_t, 60> message;
  std::iota(message.begin(), message.end(), uint8_t(0));
  XECRYPT_SHA_STATE state;
  XeCryptShaInit(&state);
  XeCryptShaUpdate(&state, message.data(), 60);

  REQUIRE(Final(state) == FromHex("c98714b16f92c8a770e9fc229df834d1688e282f"));
  std::array<uint8_t, 64> block{};
  block[62] = 0x01;
  block[63] = 0xE0;
  REQUIRE(std::memcmp(state.buffer, block.data(), block.size()) == 0);
  REQUIRE(Final(state) == FromHex("59c10a548f4ef60095e57190d6d17b48c54c90c0"));
}

TEST_CASE("XeCryptShaFinal_32bit_bit_count", "[crypt]") {
  // Only the low 32 bits of the bit count are padded in, so 0x20000003 bytes
  // pad like 3.
  XECRYPT_SHA_STATE state;
  XeCryptShaInit(&state);
  XeCryptShaUpdate(&state, Bytes("abc"), 3);
  state.count = 0x20000003;
  REQUIRE(Final(state) == FromHex("a9993e364706816aba3e25717850c26c9cd0d89d"));
}
