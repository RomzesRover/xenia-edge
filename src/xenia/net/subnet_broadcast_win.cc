/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/net/subnet_broadcast.h"

#include <algorithm>

#include "xenia/base/platform_win.h"

// iphlpapi.h needs the winsock types first.
#include <winsock2.h>  // NOLINT(build/include_order)

#include <iphlpapi.h>  // NOLINT(build/include_order)

namespace xe::net {

std::vector<uint32_t> SubnetBroadcastAddresses() {
  std::vector<uint32_t> result;
  constexpr ULONG kFlags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                           GAA_FLAG_SKIP_DNS_SERVER |
                           GAA_FLAG_SKIP_FRIENDLY_NAME;
  ULONG size = 16 * 1024;
  std::vector<uint8_t> buffer;
  ULONG status;
  do {
    buffer.resize(size);
    status = GetAdaptersAddresses(
        AF_INET, kFlags, nullptr,
        reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
  } while (status == ERROR_BUFFER_OVERFLOW);
  if (status != NO_ERROR) {
    return result;
  }
  for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
       adapter; adapter = adapter->Next) {
    if (adapter->OperStatus != IfOperStatusUp ||
        adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
      continue;
    }
    for (auto* unicast = adapter->FirstUnicastAddress; unicast;
         unicast = unicast->Next) {
      const uint8_t prefix = unicast->OnLinkPrefixLength;
      if (unicast->Address.lpSockaddr->sa_family != AF_INET || !prefix ||
          prefix > 30) {
        continue;
      }
      const auto* address =
          reinterpret_cast<const sockaddr_in*>(unicast->Address.lpSockaddr);
      const uint32_t broadcast =
          ntohl(address->sin_addr.s_addr) | (0xFFFFFFFFu >> prefix);
      if (std::find(result.begin(), result.end(), broadcast) == result.end()) {
        result.push_back(broadcast);
      }
    }
  }
  return result;
}

}  // namespace xe::net
