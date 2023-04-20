/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2023, chys <admin@CHYS.INFO>
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

#include <netinet/in.h>

#include <bit>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
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
class IPv4 {
 public:
  constexpr IPv4() noexcept : v_(0) {}
  constexpr explicit IPv4(std::uint32_t v,
                          std::endian byte_order = native_endian) noexcept
      : v_(may_bswap(byte_order, native_endian, v)) {}
  constexpr IPv4(BrokeIPv4 ip) noexcept : IPv4(ip.a, ip.b, ip.c, ip.d) {}
  constexpr IPv4(std::uint8_t a, std::uint8_t b, std::uint8_t c,
                 std::uint8_t d) noexcept
      : v_(a << 24 | b << 16 | c << 8 | d) {}
  explicit constexpr IPv4(const in_addr& addr) noexcept
      : IPv4(addr.s_addr, std::endian::big) {}
  constexpr IPv4(const IPv4&) noexcept = default;

  constexpr IPv4& operator=(const IPv4&) noexcept = default;

  constexpr std::uint32_t value(
      std::endian byte_order = native_endian) const noexcept {
    return may_bswap(byte_order, native_endian, v_);
  }

  constexpr std::uint8_t a() const noexcept { return std::uint8_t(v_ >> 24); }
  constexpr std::uint8_t b() const noexcept { return std::uint8_t(v_ >> 16); }
  constexpr std::uint8_t c() const noexcept { return std::uint8_t(v_ >> 8); }
  constexpr std::uint8_t d() const noexcept { return std::uint8_t(v_); }

  constexpr operator BrokeIPv4() const noexcept {
    return BrokeIPv4{a(), b(), c(), d()};
  }

  char* ToString(char* buffer) const noexcept;
  short_string<15> ToString() const noexcept;

  constexpr in_addr Get() const noexcept {
    in_addr res;
    res.s_addr = bswap_be(v_);
    return res;
  }

  explicit constexpr operator bool() const noexcept { return v_; }

  static IPv4 FromRaw(const void* data) noexcept {
    std::uint32_t v;
    __builtin_memcpy(&v, data, 4);
    return IPv4(v, network_endian);
  }

  void ToRaw(void* data) const noexcept {
    std::uint32_t v = value(network_endian);
    __builtin_memcpy(data, &v, 4);
  }

  // FromCommonString supports only the common "a.b.c.d" format
  static std::optional<IPv4> FromCommonString(std::string_view s) noexcept;
  // FromString supports all legal formats,
  // e.g. "127.0.0.1" may be represented as any of the following:
  // "127.0.0.1", "127.0.1", "127.1", "2130706433", "0x7f000001", "0x7f.0.0.1",
  // "017700000001", and so on...
  // However, some implementation doesn't check for overflowed values, but we
  // do it strictly.
  static std::optional<IPv4> FromString(std::string_view s) noexcept;

  static consteval IPv4 CanonicalLocalhost() noexcept {
    return IPv4(127, 0, 0, 1);
  }

  constexpr bool IsLocalhost() const noexcept { return a() == 127; }
  constexpr bool IsCanonicalLocalhost() const noexcept {
    return *this == CanonicalLocalhost();
  }

  constexpr IPv4& operator&=(const IPv4& o) noexcept {
    v_ &= o.v_;
    return *this;
  }
  constexpr IPv4& operator|=(const IPv4& o) noexcept {
    v_ |= o.v_;
    return *this;
  }

  friend inline constexpr bool operator==(const IPv4& lhs,
                                          const IPv4& rhs) noexcept {
    return lhs.v_ == rhs.v_;
  }
  friend inline constexpr std::strong_ordering operator<=>(
      const IPv4& lhs, const IPv4& rhs) noexcept {
    return lhs.v_ <=> rhs.v_;
  }
  friend inline constexpr IPv4 operator&(const IPv4& lhs,
                                         const IPv4& rhs) noexcept {
    return IPv4(lhs.v_ & rhs.v_);
  }
  friend inline constexpr IPv4 operator|(const IPv4& lhs,
                                         const IPv4& rhs) noexcept {
    return IPv4(lhs.v_ | rhs.v_);
  }

  friend std::ostream& operator<<(std::ostream& os, const IPv4& ip) {
    ip.OutputTo(os);
    return os;
  }

 private:
  void OutputTo(std::ostream& os) const;

 private:
  std::uint32_t v_;
};

} // namespace cbu

namespace std {

template <>
struct hash<::cbu::IPv4> {
  constexpr size_t operator()(const ::cbu::IPv4& ip) const noexcept {
    return ip.value();
  }
};

}  // namespace std
