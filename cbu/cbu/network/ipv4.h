/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of chys <admin@CHYS.INFO> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY chys <admin@CHYS.INFO> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "cbu/common/byteorder.h"

namespace cbu {

template <std::size_t> class short_string;

inline constexpr std::endian network_endian = std::endian::big;
inline constexpr std::endian native_endian = std::endian::native;

struct BrokeIPv4 {
  std::uint8_t a;
  std::uint8_t b;
  std::uint8_t c;
  std::uint8_t d;
};

// Represents an IPv4 address
// The storage is in network-endian
class IPv4 {
 public:
  static constexpr bool IS_NATIVE_ORDER = (native_endian == network_endian);
  static constexpr int A_SHIFT = IS_NATIVE_ORDER ? 24 : 0;
  static constexpr int B_SHIFT = IS_NATIVE_ORDER ? 16 : 8;
  static constexpr int C_SHIFT = IS_NATIVE_ORDER ? 8 : 16;
  static constexpr int D_SHIFT = IS_NATIVE_ORDER ? 0 : 24;

  constexpr IPv4() noexcept : v_(0) {}
  constexpr IPv4(std::endian byte_order, std::uint32_t v) noexcept :
    v_(may_bswap(byte_order, network_endian, v)) {}
  constexpr IPv4(BrokeIPv4 ip) noexcept : IPv4(ip.a, ip.b, ip.c, ip.d) {}
  constexpr IPv4(std::uint8_t a, std::uint8_t b,
                 std::uint8_t c, std::uint8_t d) noexcept :
    v_(a << A_SHIFT | b << B_SHIFT | c << C_SHIFT | d << D_SHIFT) {}
  constexpr IPv4(const IPv4&) noexcept = default;

  constexpr IPv4& operator=(const IPv4&) noexcept = default;

  constexpr std::uint32_t value(std::endian byte_order) const noexcept {
    return may_bswap(byte_order, network_endian, v_);
  }

  constexpr std::uint8_t a() const noexcept {
    return std::uint8_t(v_ >> A_SHIFT);
  }
  constexpr std::uint8_t b() const noexcept {
    return std::uint8_t(v_ >> B_SHIFT);
  }
  constexpr std::uint8_t c() const noexcept {
    return std::uint8_t(v_ >> C_SHIFT);
  }
  constexpr std::uint8_t d() const noexcept {
    return std::uint8_t(v_ >> D_SHIFT);
  }

  constexpr operator BrokeIPv4() const noexcept {
    return BrokeIPv4{a(), b(), c(), d()};
  }

  char* to_string(char* buffer) const noexcept;
  short_string<15> to_string() const;

  static std::optional<IPv4> from_string(std::string_view s) noexcept;

  constexpr IPv4& operator &= (const IPv4& o) noexcept {
    v_ &= o.v_;
    return *this;
  }
  constexpr IPv4& operator |= (const IPv4& o) noexcept {
    v_ |= o.v_;
    return *this;
  }

  friend inline constexpr bool operator==(const IPv4& lhs,
                                          const IPv4& rhs) noexcept {
    return lhs.v_ == rhs.v_;
  }
  friend inline constexpr IPv4 operator&(const IPv4& lhs,
                                         const IPv4& rhs) noexcept {
    return IPv4(network_endian, lhs.v_ & rhs.v_);
  }
  friend inline constexpr IPv4 operator|(const IPv4& lhs,
                                         const IPv4& rhs) noexcept {
    return IPv4(network_endian, lhs.v_ | rhs.v_);
  }

 private:
  std::uint32_t v_;
};

} // namespace cbu
