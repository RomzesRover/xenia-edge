/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_NET_SUBNET_BROADCAST_H_
#define XENIA_NET_SUBNET_BROADCAST_H_

#include <cstdint>
#include <vector>

namespace xe::net {

// The broadcast address of each IPv4 interface that is up, loopback aside, in
// host byte order.
std::vector<uint32_t> SubnetBroadcastAddresses();

}  // namespace xe::net

#endif  // XENIA_NET_SUBNET_BROADCAST_H_
