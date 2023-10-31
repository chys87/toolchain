/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022-2023, chys <admin@CHYS.INFO>
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

#include "cbu/network/ipv6.h"

#include <string.h>
#include <netinet/in.h>
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif
#ifdef __ARM_NEON
#  include <arm_neon.h>
#endif

#include "cbu/common/bit.h"
#include "cbu/common/byteorder.h"
#include "cbu/common/faststr.h"
#include "cbu/common/short_string.h"
#include "cbu/math/fastdiv.h"
#include "cbu/network/ipv4.h"

namespace cbu {
namespace {

inline int FormatLastTwoFieldsAsIPv4(const in6_addr& addr) noexcept {
  uint64_t u64_0_be = mempick<uint64_t>(addr.s6_addr);
  uint32_t u32_2_be = addr.s6_addr32[2];

  // ::ffff:0:0/96
  if (u64_0_be == 0 && u32_2_be == bswap_be<uint32_t>(0x0000ffff)) return 1;
  // ::ffff:0:0:0/96
  if (u64_0_be == 0 && u32_2_be == bswap_be<uint32_t>(0xffff0000)) return 2;
  // 64:ff9b::/96
  if (u64_0_be == bswap_be<uint64_t>(0x64'ff9b'00000000) && u32_2_be == 0)
    return 2;
  // 64:ff9b:1::/48
  if ((u64_0_be & bswap_be<uint64_t>(0xffff'ffff'ffff'0000)) ==
      bswap_be<uint64_t>(0x64'ff9b'0001'0000))
    return 2;
  return 0;
}

inline char FormatChar(uint8_t c) noexcept { return "0123456789abcdef"[c]; }

inline char* FormatField(char* w, uint16_t v) noexcept {
#if defined __SSE4_1__ && defined __BMI2__
  uint32_t vext = bswap(_pdep_u32(v, 0x0f0f0f0f));
  uint32_t skip_bytes = cbu::ctz(vext | 0x8000'0000) / 8;
  __m128i chars_v =
      _mm_shuffle_epi8(*(const __m128i_u*)"0123456789abcdef",
                       _mm_cvtsi32_si128(vext >> (skip_bytes * 8)));
  memdrop(w, __v4si(chars_v)[0]);
  return w + 4 - skip_bytes;
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t skip_bytes = (cbu::clz(uint32_t(v | 1)) - 16) / 4;
  uint32_t vext =
      ((v & 0xf) << 24) | ((v & 0xf0) << 12) | (v & 0xf00) | (v >> 12);
  uint8x8_t chars_v =
      vqtbl1_u8(*(const uint8x16_t*)"0123456789abcdef",
                vreinterpret_u8_u32(vmov_n_u32(vext >> (skip_bytes * 8))));
  memdrop(w, vreinterpret_u32_u8(chars_v)[0]);
  return w + 4 - skip_bytes;
#endif
  if (v >= 0x1000) *w++ = FormatChar(v >> 12);
  if (v >= 0x100) *w++ = FormatChar((v >> 8) & 0xf);
  if (v >= 0x10) *w++ = FormatChar((v >> 4) & 0xf);
  *w++ = FormatChar(v & 0xf);
  return w;
}

}  // namespace

char* IPv6::Format(char* w, const in6_addr& addr, int flags) noexcept {
  const uint16_t(&u16)[8] = addr.s6_addr16;
  const uint32_t(&u32)[4] = addr.s6_addr32;

  int ipv4 = FormatLastTwoFieldsAsIPv4(addr);

  if ((flags & kPreferBareIPv4) == 0 || ipv4 != 1) {
#ifdef __SSE2__
    __m128i v6 =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(addr.s6_addr));
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint16x8_t v6 = *reinterpret_cast<const uint16x8_t*>(addr.s6_addr);
#endif

    uint32_t zero_mask = 0;

#if defined __AVX512VL__ && defined __AVX512BW__
    zero_mask = _mm_cmpeq_epi16_mask(_mm_setzero_si128(), v6);
#elif defined __SSE2__
    {
      __m128i v = _mm_cmpeq_epi16(_mm_setzero_si128(), v6);
      v = _mm_packs_epi16(v, _mm_setzero_si128());
      zero_mask = _mm_movemask_epi8(v);
    }
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    {
      uint16x8_t v =
          uint16x8_t(v6 == 0) & uint16x8_t{1, 2, 4, 8, 16, 32, 64, 128};
      zero_mask = vaddvq_u16(v);
    }
#else
    for (size_t i = 0; i < 8; ++i)
      if (u16[i] == 0) zero_mask |= 1 << i;
#endif

    if (ipv4) zero_mask &= (1u << 6) - 1;

    uint32_t compress_pos = 8;

    // The rule: Compress the longest run of zero fields.
    // If there are multiple runs of the same length, compress the leftmost.
    if (zero_mask) {
      // Don't check for more than 4 fields, since the total is 8.
      // So the first run of 4 zero fields must be the starting point of
      // compression.
      uint32_t two_zeros = zero_mask & (zero_mask >> 1);
      uint32_t three_zeros = two_zeros & (zero_mask >> 2);
      uint32_t four_zeros = three_zeros & (zero_mask >> 3);

      compress_pos = ctz(four_zeros    ? four_zeros
                         : three_zeros ? three_zeros
                         : two_zeros   ? two_zeros
                                       : zero_mask);
    }

    {
#if defined __AVX512VL__ && defined __AVX512BW__ && defined __AVX512CD__
      uint64_t skip_bytes_u64;
      union {
        char formatted_bytes[8][4];
        __m256i_u formatted_bytes_256;
      };
      __m256i vx6 = _mm256_cvtepu16_epi32(_mm_shuffle_epi8(
          v6,
          _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14)));
      __m256i v_skip_raw_bits = _mm256_min_epu32(
          _mm256_sub_epi32(_mm256_lzcnt_epi32(vx6), _mm256_set1_epi32(16)),
          _mm256_set1_epi32(12));
      __m256i v_skip_bytes = _mm256_srli_epi32(v_skip_raw_bits, 2);
      __m256i v_skip_real_bits = _mm256_slli_epi32(v_skip_bytes, 3);
      skip_bytes_u64 = _mm_cvtsi128_si64(_mm256_cvtepi32_epi8(v_skip_bytes));

      __m256i vxx =
          __m256i(((__v8su(vx6) & 0xf) << 24) | ((__v8su(vx6) & 0xf0) << 12) |
                  ((__v8su(vx6) & 0xf00)) | (__v8su(vx6) >> 12));
      formatted_bytes_256 = _mm256_srlv_epi32(
          _mm256_shuffle_epi8(
              *(const __m256i_u*)"0123456789abcdef0123456789abcdef", vxx),
          __m256i(v_skip_real_bits));
#endif

      uint32_t i = 0;
      uint32_t limit = ipv4 ? 6 : 8;
      while (i < limit) {
        if (i == compress_pos) {
          if (i == 0) *w++ = ':';
          uint32_t next_i = ctz(~zero_mask & ~((1u << i) - 1));
#if defined __AVX512VL__ && defined __AVX512BW__ && defined __AVX512CD__
          skip_bytes_u64 >>= (8 * (next_i - i));
#endif
          i = next_i;
          *w++ = ':';
        } else {
#if defined __AVX512VL__ && defined __AVX512BW__ && defined __AVX512CD__
          if (true) {
            memcpy(w, formatted_bytes[i], 4);
            w += 4l - uint8_t(skip_bytes_u64);
          } else
#endif
          {
            w = FormatField(w, bswap_be(u16[i]));
          }
          if (++i < 8) *w++ = ':';
#if defined __AVX512VL__ && defined __AVX512BW__ && defined __AVX512CD__
          skip_bytes_u64 >>= 8;
#endif
        }
      }
    }
  }

  if (ipv4) w = IPv4(u32[3], std::endian::big).ToString(w);
  return w;
}

short_string<IPv6::kMaxStringLen> IPv6::Format(const in6_addr& addr6,
                                               int flags) noexcept {
  short_string<kMaxStringLen> res(kUninitialized);
  char* w = Format(res.buffer(), addr6, flags);
  *w = '\0';
  res.set_length(w - res.buffer());
  return res;
}

short_string<IPv6::kMaxStringLen> IPv6::ToString(int flags) const noexcept {
  return Format(a_, flags);
}

namespace {

template <typename T>
struct ParseResult {
  bool ok;
  T v;
  const char* s;
};


ParseResult<uint16_t> parse_hex_uint16(const char* s, const char* e) noexcept {
  if (s >= e) [[unlikely]]
    return {false};
  unsigned res = 0;
  for (int i = 0; i < 4 && s < e; ++i) {
    unsigned char c = *s;
    if (c >= '0' && c <= '9') {
      res = res * 16 + (c - '0');
    } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
      res = res * 16 + ((c | 0x20) - 'a' + 10);
    } else {
      if (i == 0) [[unlikely]]
        return {false};
      break;
    }
    ++s;
  }
  return {true, uint16_t(res), s};
}

}  // namespace

std::optional<IPv6> IPv6::FromString(std::string_view s) noexcept {
  // First check if it's a valid IPv4 address
  if (std::optional<IPv4> ipv4_opt = IPv4::FromCommonString(s))
    return IPv6(*ipv4_opt);

  // Surrounded by [ ]
  if (s.size() >= 2 && s.starts_with('[') && s.ends_with(']'))
    s = s.substr(1, s.size() - 2);

  in6_addr addr;
  uint16_t* parts = addr.s6_addr16;
  int omit_pos = -1;
  unsigned k = 0;

  const char* p = s.data();
  const char* e = p + s.size();

  // Special case: starts with '::'
  if (p < e && *p == ':') {
    ++p;
    if (p >= e || *p != ':') [[unlikely]]
      return std::nullopt;
    ++p;
    omit_pos = 0;
  }

  while (p < e) {
    if (k >= 8) [[unlikely]]
      return std::nullopt;
    auto [ok, v, endptr] = parse_hex_uint16(p, e);
    if (!ok) [[unlikely]]
      return std::nullopt;
    // Check for special case: IPv4 mapped IPv6
    if (endptr < e && *endptr == '.') {
      if (k > 6 || (k < 6 && omit_pos < 0)) [[unlikely]]
        return std::nullopt;
      std::optional<IPv4> ipv4_opt = IPv4::FromCommonString({p, e});
      if (!ipv4_opt) [[unlikely]]
        return std::nullopt;
      uint32_t ipv4_be = ipv4_opt->value(std::endian::big);
      memcpy(parts + k, &ipv4_be, 4);
      k += 2;
      break;
    }

    parts[k++] = bswap_be(uint16_t(v));
    p = endptr;
    if (p >= e) break;
    if (k >= 8) [[unlikely]]
      return std::nullopt;
    if (*p++ != ':') [[unlikely]]
      return std::nullopt;
    if (p < e && *p == ':') {
      if (omit_pos >= 0) [[unlikely]]
        return std::nullopt;
      ++p;
      omit_pos = k;
    }
  }

  if (k != 8) {
    if (omit_pos < 0) [[unlikely]]
      return std::nullopt;
    unsigned omit_cnt = 8 - k;
    unsigned move_cnt = k - omit_pos;
#if defined __AVX512BW__ && defined __AVX512VL__
    __m128i v = _mm_maskz_loadu_epi16(-1u << (8 - move_cnt), parts - omit_cnt);
    _mm_mask_storeu_epi16(parts, -1u << omit_pos, v);
#else
    memmove(parts + 8 - move_cnt, parts + omit_pos,
            move_cnt * sizeof(uint16_t));
    memset(parts + omit_pos, 0, omit_cnt * sizeof(uint16_t));
#endif
  }
  return IPv6(addr);
}

char* IPv6Port::PortToString(char* buf, uint16_t port) noexcept {
  unsigned myriads = cbu::fastdiv<10000, 65536>(port);
  unsigned rem = port - 10000 * myriads;
  unsigned thousands = cbu::fastdiv<1000, 10000>(rem);
  rem -= thousands * 1000;
  unsigned hundreds = cbu::fastdiv<100, 1000>(rem);
  rem -= hundreds * 100;
  unsigned tens = cbu::fastdiv<10, 100>(rem);
  unsigned ones = rem - tens * 10;

  uint64_t str = myriads | (thousands << 8) | (hundreds << 16) | (tens << 24) |
                 uint64_t(ones) << 32;

  unsigned skip_bytes = ctz(str | (1ull << 32)) / 8;
  str >>= skip_bytes * 8;

  str |= 0x30303030'30303030ull;

  memdrop_le<uint32_t>(buf, str);
  buf[4] = str >> 32;

  return buf + 5 - skip_bytes;
}

char* IPv6Port::ToString(char* buf, int flags) const noexcept {
  if ((flags & kPreferBareIPv4) && ip.IsIPv4()) {
    buf = ip.GetIPv4().ToString(buf);
  } else {
    *buf++ = '[';
    buf = ip.ToString(buf);
    *buf++ = ']';
  }
  *buf++ = ':';
  buf = PortToString(buf, port);
  return buf;
}

short_string<IPv6Port::kMaxStringLen> IPv6Port::ToString(
    int flags) const noexcept {
  short_string<kMaxStringLen> res(kUninitialized);
  char* w = ToString(res.buffer(), flags);
  *w = '\0';
  res.set_length(w - res.buffer());
  return res;
}

}  // namespace cbu
