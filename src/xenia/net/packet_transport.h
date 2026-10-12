/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_NET_PACKET_TRANSPORT_H_
#define XENIA_NET_PACKET_TRANSPORT_H_

#include <cstdint>
#include <span>
#include <vector>

#include "xenia/net/xnet_secure.h"

namespace xe::net::secure {

// Carries this console's Ethernet frames to and from the System Link LAN.
class PacketTransport {
 public:
  virtual ~PacketTransport() = default;
  virtual Mac identity() const = 0;
  virtual uint32_t mtu() const = 0;
  virtual bool healthy() const { return true; }
  // Returns the bytes sent or -1.
  virtual int Send(std::span<const uint8_t> frame) = 0;
  // False on timeout or error.
  virtual bool Receive(std::vector<uint8_t>& frame, uint32_t timeout_ms) = 0;
};

}  // namespace xe::net::secure

#endif  // XENIA_NET_PACKET_TRANSPORT_H_
