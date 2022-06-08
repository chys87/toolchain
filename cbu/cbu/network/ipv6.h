/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022, chys <admin@CHYS.INFO>
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

#if __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif

#include <stddef.h>
#include <string.h>
#include <netinet/in.h>

#include <algorithm>
#include <compare>
#include <string_view>

#include "cbu/common/byteorder.h"
#include "cbu/common/faststr.h"
#include "cbu/network/ipv4.h"

namespace cbu {

template <size_t>
class short_string;

class IPv6 {
 public:
  // strlen("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") == 39
  // strlen("0064:ff9b:0001:ffff:ffff:ffff:255.255.255.255") == 45
  static inline constexpr size_t kMaxStringLen = INET6_ADDRSTRLEN - 1;
  static char* Format(char* buf, const in6_addr& addr6) noexcept;
  static short_string<kMaxStringLen> Format(const in6_addr& addr6) noexcept;

 public:
  constexpr IPv6() noexcept : a_() {}
  constexpr IPv6(const in6_addr& addr6) noexcept : a_(addr6) {}

  constexpr IPv6(const in_addr& addr4) noexcept {
    std::fill_n(a_.s6_addr, 10, 0);
    a_.s6_addr[10] = 0xff;
    a_.s6_addr[11] = 0xff;
    memdrop(a_.s6_addr + 12, addr4.s_addr);
  }
  constexpr IPv6(const IPv6&) noexcept = default;
  constexpr IPv6(const IPv4& ip)noexcept {
    std::fill_n(a_.s6_addr, 10, 0);
    a_.s6_addr[10] = 0xff;
    a_.s6_addr[11] = 0xff;
    memdrop_be(a_.s6_addr + 12, ip.value());
  }

  constexpr IPv6& operator=(const IPv6&) noexcept = default;

  char* ToString(char* buf) const noexcept { return Format(buf, a_); }
  short_string<kMaxStringLen> ToString() const noexcept;

  constexpr const in6_addr& Get() const noexcept { return a_; }

  constexpr bool IsIPv4() const noexcept {
    return mempick<uint64_t>(a_.s6_addr) == 0 &&
           mempick<uint32_t>(a_.s6_addr + 8) == bswap_be(0x0000ffffu);
  }

  constexpr IPv4 GetIPv4() const noexcept {
    return IPv4(mempick<uint32_t>(a_.s6_addr + 12), std::endian::big);
  }

  constexpr in_addr GetIPv4Addr() const noexcept {
    in_addr r;
    r.s_addr = mempick<uint32_t>(a_.s6_addr + 12);
    return r;
  }

  friend constexpr bool operator==(const IPv6& a, const IPv6& b) noexcept {
    if consteval {
      return std::equal(std::begin(a.a_.s6_addr), std::end(a.a_.s6_addr),
                        b.a_.s6_addr);
    } else {
#ifdef __SSE4_2__
      __m128i v = a.ToVec() ^ b.ToVec();
      return _mm_testz_si128(v, v);
#endif
      return memcmp(a.a_.s6_addr, b.a_.s6_addr, 16) == 0;
    }
  }

  friend constexpr std::strong_ordering operator<=>(const IPv6& a,
                                                    const IPv6& b) noexcept {
    std::strong_ordering r = mempick_be<uint64_t>(a.a_.s6_addr) <=>
                             mempick_be<uint64_t>(b.a_.s6_addr);
    if (r == 0)
      r = mempick_be<uint64_t>(a.a_.s6_addr + 8) <=>
          mempick_be<uint64_t>(b.a_.s6_addr + 8);
    return r;
  }

  constexpr size_t Hash() const noexcept {
    if constexpr (sizeof(size_t) < 8) {
      return std::hash<std::string_view>()(std::string_view(
          reinterpret_cast<const char*>(a_.s6_addr), sizeof(a_.s6_addr)));
    } else {
      uint64_t a = mempick<uint64_t>(a_.s6_addr);
      uint64_t b = mempick<uint64_t>(a_.s6_addr + 8);
      // This is a big prime
      return a * 14029467366897019727ULL ^ b;
    }
  }

 private:
#ifdef __SSE4_2__
  __m128i ToVec() const noexcept {
    return *reinterpret_cast<const __m128i_u*>(a_.s6_addr);
  }
#endif

 private:
  // We want IPv6 to be a literal type, so we try out best to use only a_.s6_addr
  // without a_.s6_addr16/a_.s6_addr32 in constxpr context
  in6_addr a_;
};

struct IPv6Port {
  IPv6 ip;
  uint16_t port;

  static constexpr IPv6Port FromSockAddr(const sockaddr_in6& s) {
    return {IPv6(s.sin6_addr), bswap_be(s.sin6_port)};
  }
  static constexpr IPv6Port FromSockAddr(const sockaddr_in& s) {
    return {IPv6(s.sin_addr), bswap_be(s.sin_port)};
  }

  // This cannot be constexpr
  static IPv6Port FromSockAddr(const sockaddr* s, socklen_t l) {
    if (s->sa_family == AF_INET6)
      return FromSockAddr(*reinterpret_cast<const sockaddr_in6*>(s));
    else if (s->sa_family == AF_INET)
      return FromSockAddr(*reinterpret_cast<const sockaddr_in*>(s));
    else
      return {};
  }

  constexpr void ToSockAddr(sockaddr_in6* s) const noexcept {
    *s = {};
    s->sin6_family = AF_INET6;
    s->sin6_addr = ip.Get();
    s->sin6_port = bswap_be<uint16_t>(port);
  }

  static inline constexpr size_t kMaxStringLen = IPv6::kMaxStringLen + 8;
  static char* PortToString(char* buf, uint16_t port) noexcept;

  char* ToString(char* buf) const noexcept;
  short_string<kMaxStringLen> ToString() const noexcept;

  constexpr size_t Hash() const noexcept {
    uint64_t h = ip.Hash();
    return h * 3266489917 ^ port;
  }

  friend constexpr bool operator==(const IPv6Port& a,
                                   const IPv6Port& b) noexcept {
    return a.ip == b.ip && a.port == b.port;
  }
  friend constexpr std::strong_ordering operator<=>(
      const IPv6Port& a, const IPv6Port& b) noexcept {
    std::strong_ordering r = a.ip <=> b.ip;
    if (r == 0) r = a.port <=> b.port;
    return r;
  }
};

}  // namespace cbu

namespace std {

template <>
struct hash<::cbu::IPv6> {
  constexpr size_t operator()(const ::cbu::IPv6& ip) const noexcept {
    return ip.Hash();
  }
};

template <>
struct hash<::cbu::IPv6Port> {
  constexpr size_t operator()(const ::cbu::IPv6Port& ip_port) const noexcept {
    return ip_port.Hash();
  }
};

}  // namespace std
