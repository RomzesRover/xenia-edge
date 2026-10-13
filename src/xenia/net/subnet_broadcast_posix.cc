/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/net/subnet_broadcast.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <algorithm>
#include <cstring>

#include "xenia/base/platform.h"

namespace xe::net {
namespace {

uint32_t Ipv4(const sockaddr* address) {
  sockaddr_in full{};
#if XE_PLATFORM_MAC
  // BSD trims trailing zero bytes off a netmask, so it can end early.
  std::memcpy(&full, address, std::min<size_t>(address->sa_len, sizeof(full)));
#else
  std::memcpy(&full, address, sizeof(full));
#endif
  return ntohl(full.sin_addr.s_addr);
}

}  // namespace

std::vector<uint32_t> SubnetBroadcastAddresses() {
  std::vector<uint32_t> result;
  ifaddrs* interfaces = nullptr;
  if (getifaddrs(&interfaces)) {
    return result;
  }
  for (auto* entry = interfaces; entry; entry = entry->ifa_next) {
    if (!entry->ifa_addr || !entry->ifa_netmask ||
        entry->ifa_addr->sa_family != AF_INET || !(entry->ifa_flags & IFF_UP) ||
        !(entry->ifa_flags & IFF_RUNNING) ||
        !(entry->ifa_flags & IFF_BROADCAST) ||
        (entry->ifa_flags & IFF_LOOPBACK)) {
      continue;
    }
    const uint32_t host_bits = ~Ipv4(entry->ifa_netmask);
    // A /31 or /32 has no broadcast address and a /0 would give the limited
    // one.
    if (host_bits <= 1 || host_bits == 0xFFFFFFFF) {
      continue;
    }
    const uint32_t broadcast = Ipv4(entry->ifa_addr) | host_bits;
    if (std::find(result.begin(), result.end(), broadcast) == result.end()) {
      result.push_back(broadcast);
    }
  }
  freeifaddrs(interfaces);
  return result;
}

}  // namespace xe::net
