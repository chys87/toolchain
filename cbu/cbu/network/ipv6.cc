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

#include "cbu/network/ipv6.h"

#include <string.h>
#include <netinet/in.h>
#if __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif

#include "cbu/common/bit.h"
#include "cbu/common/byteorder.h"
#include "cbu/common/faststr.h"
#include "cbu/common/short_string.h"
#include "cbu/network/ipv4.h"

namespace cbu {
namespace {

inline bool FormatLastTwoFieldsAsIPv4(const in6_addr& addr) noexcept {
  uint64_t u64_0_be = mempick<uint64_t>(addr.s6_addr);
  uint32_t u32_2_be = addr.s6_addr32[2];

  // ::ffff:0:0/96
  if (u64_0_be == 0 && u32_2_be == bswap_be<uint32_t>(0x0000ffff)) return true;
  // ::ffff:0:0:0/96
  if (u64_0_be == 0 && u32_2_be == bswap_be<uint32_t>(0xffff0000)) return true;
  // 64:ff9b::/96
  if (u64_0_be == bswap_be<uint64_t>(0x64'ff9b'00000000) && u32_2_be == 0)
    return true;
  // 64::ff9b:1::/48
  if ((u64_0_be & bswap_be<uint64_t>(0xffff'ffff'ffff'0000)) ==
      bswap_be<uint64_t>(0x64'ff9b'0001'0000))
    return true;
  return false;
}

inline char FormatChar(uint8_t c) noexcept { return "0123456789abcdef"[c]; }

inline char* FormatField(char* w, uint16_t v) noexcept {
  if (v >= 0x1000) *w++ = FormatChar(v >> 12);
  if (v >= 0x100) *w++ = FormatChar((v >> 8) & 0xf);
  if (v >= 0x10) *w++ = FormatChar((v >> 4) & 0xf);
  *w++ = FormatChar(v & 0xf);
  return w;
}

}  // namespace

char* IPv6::Format(char* w, const in6_addr& addr) noexcept {
  const uint16_t(&u16)[8] = addr.s6_addr16;
  const uint32_t(&u32)[4] = addr.s6_addr32;

  uint32_t zero_mask = 0;
#ifdef __SSE2__
  {
    __m128i v = _mm_cmpeq_epi16(
        _mm_setzero_si128(),
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(addr.s6_addr)));
    v = _mm_packs_epi16(v, _mm_setzero_si128());
    zero_mask = _mm_movemask_epi8(v);
  }
#else
  for (size_t i = 0; i < 8; ++i)
    if (u16[i] == 0) zero_mask |= 1 << i;
#endif

  bool ipv4 = FormatLastTwoFieldsAsIPv4(addr);
  if (ipv4) zero_mask &= (1u << 6) - 1;

  uint32_t compress_pos = 8;

  // The rule: Compress the longest run of zero fields.
  // If there are multiple runs of the same length, compress the leftmost.
  if (zero_mask) {
    compress_pos = ctz(zero_mask);
    if (uint32_t two_zeros = zero_mask & (zero_mask >> 1)) {
      compress_pos = ctz(two_zeros);
      if (uint32_t three_zeros = two_zeros & (zero_mask >> 2)) {
        compress_pos = ctz(three_zeros);
        if (uint32_t four_zeros = three_zeros & (zero_mask >> 3)) {
          compress_pos = ctz(four_zeros);
          // Don't check for more than 4 fields, since the total is 8.
          // So the first run of 4 zero fields must be the starting point of
          // compression.
        }
      }
    }
  }

  uint32_t i = 0;
  while (i < (ipv4 ? 6 : 8)) {
    if (i == compress_pos) {
      if (i == 0) *w++ = ':';
      i = ctz(~zero_mask & ~((1u << i) - 1));
      *w++ = ':';
    } else {
      w = FormatField(w, bswap_be(u16[i]));
      if (++i < 8) *w++ = ':';
    }
  }

  if (ipv4) w = IPv4(u32[3], std::endian::big).to_string(w);
  return w;
}

short_string<IPv6::kMaxStringLen> IPv6::Format(const in6_addr& addr6) noexcept {
  short_string<kMaxStringLen> res;
  char* w = Format(res.buffer(), addr6);
  res.set_length(w - res.data());
  return res;
}

}  // namespace cbu
