/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2024, chys <admin@CHYS.INFO>
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

#include "cbu/network/ipv4.h"

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif
#ifdef __ARM_NEON
#  include <arm_neon.h>
#endif

#include <limits>

#include "cbu/math/common.h"
#include "cbu/math/fastdiv.h"
#include "cbu/strings/faststr.h"
#include "cbu/strings/short_string.h"

namespace cbu {
namespace {

inline char* uint8_to_string(char* p, std::uint8_t v) noexcept {
  if (v >= 100) {
    if (v >= 200) {
      *p++ = '2';
      v -= 200;
    } else {
      *p++ = '1';
      v -= 100;
    }
    goto _j;
  }
  if (v >= 10) {
_j:
    *p++ = fastdiv<10, 100>(v) + '0';
    v = fastmod<10, 100>(v);
  }
  *p++ = v + '0';
  return p;
}

template <typename T>
struct ParseResult {
  bool ok;
  T v;
  const char* s;
};

[[gnu::always_inline]] inline ParseResult<std::uint8_t> parse_uint8(
    const char* s, const char* e) noexcept {
  if (s >= e) {
    return {false};
  }
  unsigned int x = std::uint8_t(*s++) - '0';
  if (x == 0) {
    // 0 is a special case.  We don't recognize extra leading zeros
    return {true, 0, s};
  }
  if (x >= 10) {
    return {false};
  }
  unsigned int t;
  if (s < e && (t = std::uint8_t(*s) - '0') < 10) {
    x = x * 10 + t;
    if (++s < e && (t = std::uint8_t(*s) - '0') < 10) {
      x = x * 10 + t;
      ++s;
      if (std::uint8_t(x) != x) return {false};
    }
  }
  return {true, std::uint8_t(x), s};
}

inline std::optional<unsigned> parse_hex_digit(unsigned char c) {
  static_assert('A' == 65, "This implementation requires ASCII");
  if (unsigned C = c - '0'; C < 10)
    return C;
  else if (unsigned C = (c | 0x20) - 'a'; C < 6)
    return C + 10;
  else
    return std::nullopt;
}

ParseResult<std::uint32_t> parse_any_uint32(const char* s,
                                            const char* e) noexcept {
  if (s >= e) return {false};
  if (*s >= '1' && *s <= '9') [[likely]] {
    // Decimal
    std::uint32_t r = *s++ - '0';
    while (s < e && (*s >= '0' && *s <= '9')) {
      if (r > std::numeric_limits<std::uint32_t>::max() / 10) return {false};
      r *= 10;
      if (add_overflow(r, *s++ - '0', &r)) return {false};
    }
    return {true, r, s};
  } else if (*s == '0') {
    ++s;
    if (s >= e) {
      // simply 0
      return {true, 0, s};
    }
    if (*s == 'x' || *s == 'X') {
      // Hexadecimal
      ++s;
      auto first_digit_opt = parse_hex_digit(*s);
      if (!first_digit_opt) return {false};
      std::uint32_t r = *first_digit_opt;
      while (++s < e) {
        auto digit_opt = parse_hex_digit(*s);
        if (!digit_opt) break;
        // Overflow?
        if (r & ~(std::uint32_t(-1) >> 4)) return {false};
        r = (r << 4) + *digit_opt;
      }
      return {true, r, s};
    } else {
      // Octal or simply "0"
      std::uint32_t r = 0;
      while (s < e && *s >= '0' && *s <= '7') {
        // Overflow?
        if (r & ~(std::uint32_t(-1) >> 3)) return {false};
        r = (r << 3) + (*s++ - '0');
      }
      return {true, r, s};
    }
  } else {
    return {false};
  }
}

} // namespace

char* IPv4::ToString(char* p) const noexcept {
#ifdef __SSE4_1__
  __v4su v = __v4su(_mm_cvtepu8_epi32(_mm_cvtsi32_si128(v_)));
  __v4su hundreds = (v * 41) >> 12;
  __v4su rem = v - hundreds * 100;
  __v4su tens = (rem * 103) >> 10;
  __v4su ones = rem - tens * 10;
  __v4su combined = hundreds | (tens << 8) | (ones << 16);

  __v4su skipped_bytes = __v4su(((__v4si(v) < 10) & 1) + ((__v4si(v) < 100) & 1));

  __v4su bytes = __v4su(_mm_srlv_epi32(__m128i(combined | 0x2e303030),
                                       __m128i(skipped_bytes * 8)));
  *(uint32_t*)p = bytes[3];
  p -= skipped_bytes[3];
  *(uint32_t*)(p + 4) = bytes[2];
  p -= skipped_bytes[2];
  *(uint32_t*)(p + 8) = bytes[1];
  p -= skipped_bytes[1];
  *(uint16_t*)(p + 12) = __v8hu(bytes)[0];
  *(p + 14) = __v16qu(bytes)[2];
  p += 15l - skipped_bytes[0];
  return p;
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32x4_t v =
      vmovl_u16(vget_low_u16(vmovl_u8(vreinterpret_u8_u32(uint32x2_t{v_, 0}))));
  uint32x4_t hundreds = (v * 41) >> 12;
  uint32x4_t rem = v - hundreds * 100;
  uint32x4_t tens = (rem * 103) >> 10;
  uint32x4_t ones = rem - tens * 10;
  uint32x4_t combined = hundreds | (tens << 8) | (ones << 16);

  int32x4_t skipped_bytes = int32x4_t((v < 10) + (v < 100));

  uint32x4_t bytes =
      vshlq_u32(combined | vdupq_n_u32(0x2e303030), skipped_bytes * 8);

  *(uint32_t*)p = bytes[3];
  p += skipped_bytes[3];
  *(uint32_t*)(p + 4) = bytes[2];
  p += skipped_bytes[2];
  *(uint32_t*)(p + 8) = bytes[1];
  p += skipped_bytes[1];
  *(uint16_t*)(p + 12) = vreinterpretq_u16_u32(bytes)[0];
  *(p + 14) = vreinterpretq_u8_u32(bytes)[2];
  p += 15l + skipped_bytes[0];
  return p;
#endif
  p = uint8_to_string(p, a());
  *p++ = '.';
  p = uint8_to_string(p, b());
  *p++ = '.';
  p = uint8_to_string(p, c());
  *p++ = '.';
  p = uint8_to_string(p, d());
  return p;
}

short_string<15> IPv4::ToString() const noexcept {
  short_string<15> res(kUninitialized);  // The max length of an IPv4 is 15
  char* w = ToString(res.buffer());
  *w = '\0';
  res.set_length(w - res.buffer());
  return res;
}

IPv4::StringToIPv4Result IPv4::FromCommonStringImpl(const char* p,
                                                    const char* e) noexcept {
  auto [a_ok, a, a_s] = parse_uint8(p, e);
  if (!a_ok) return {false};
  p = a_s;
  uint32_t ip = a;

  if (p >= e || *p++ != '.') return {false};
  auto [b_ok, b, b_s] = parse_uint8(p, e);
  if (!b_ok) return {false};
  p = b_s;
  ip = ip * 256 + b;

  if (p >= e || *p++ != '.') return {false};
  auto [c_ok, c, c_s] = parse_uint8(p, e);
  if (!c_ok) return {false};
  p = c_s;
  ip = ip * 256 + c;

  if (p >= e || *p++ != '.') return {false};
  auto [d_ok, d, d_s] = parse_uint8(p, e);
  if (!d_ok || d_s != e) return {false};
  ip = ip * 256 + d;

  return {true, ip};
}

std::optional<IPv4> IPv4::FromString(std::string_view s) noexcept {
  if (s.empty()) return std::nullopt;

  const char* p = s.data();
  const char* e = p + s.size();

  std::uint32_t ip = 0;
  std::uint32_t unfilled_bits = 32;

  for (;;) {
    auto [ok, v, endptr] = parse_any_uint32(p, e);
    p = endptr;
    if (!ok) return std::nullopt;

    if (p >= e) {
      // Last component
      if (unfilled_bits < 32 && (v & (-1u << unfilled_bits)))
        return std::nullopt;
      ip += v;
      break;
    } else if (*p == '.' && unfilled_bits >= 16) {
      ++p;
      if (v >= 256) return std::nullopt;
      ip |= (v << (unfilled_bits -= 8));
    } else {
      return std::nullopt;
    }
  }

  return IPv4(ip);
}

void IPv4::OutputTo(std::ostream& os) const {
  char buffer[16];
  char* p = ToString(buffer);
  os << std::string_view(buffer, p - buffer);
}

} // namespace cbu
