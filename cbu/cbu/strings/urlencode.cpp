/*
 * cbu - chys's basic utilities
 * Copyright (c) 2026, chys <admin@CHYS.INFO>
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

#include "cbu/strings/urlencode.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <cstdint>
#include <optional>

#include "cbu/common/bit.h"
#include "cbu/strings/hex.h"

namespace cbu {
namespace {

#ifdef __ARM_NEON
// Convert a byte mask (0x00 or 0xff in each byte) to a 16-bit bitmask
inline unsigned neon_movemask(uint8x16_t m) noexcept {
  static constexpr std::uint8_t kBitMask[16] = {
      1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128,
  };
  uint8x16_t bits = vandq_u8(m, vld1q_u8(kBitMask));
  uint64x2_t sum = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(bits)));
  return static_cast<unsigned>(vgetq_lane_u64(sum, 0) |
                               (vgetq_lane_u64(sum, 1) << 8));
}
#endif

}  // namespace

char* decode_uri_component(char* dst, const char* src,
                           std::size_t size) noexcept {
  const char* src_end = src + size;

  while (src != src_end) {
#if defined __SSE2__ || defined __ARM_NEON
    // Fast path: keep processing 16-byte blocks as long as no '%' is found
    while (static_cast<std::size_t>(src_end - src) >= 16) {
#if defined __SSE2__
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
      unsigned mask = static_cast<unsigned>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('%'))));
      // Store the entire 16-byte block even if it contains '%': the bytes
      // after the first '%' are provisional and will be overwritten later.
      // (dst and src are guaranteed not to overlap.)
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), v);
      if (mask == 0) {
        src += 16;
        dst += 16;
        continue;
      }
      unsigned advance = ctz(mask);
      src += advance;
      dst += advance;
      break;
#else  // __ARM_NEON
      uint8x16_t v = vld1q_u8(reinterpret_cast<const std::uint8_t*>(src));
      unsigned mask = neon_movemask(vceqq_u8(v, vdupq_n_u8('%')));
      // Store the entire 16-byte block even if it contains '%': the bytes
      // after the first '%' are provisional and will be overwritten later.
      // (dst and src are guaranteed not to overlap.)
      vst1q_u8(reinterpret_cast<std::uint8_t*>(dst), v);
      if (mask == 0) {
        src += 16;
        dst += 16;
        continue;
      }
      unsigned advance = ctz(mask);
      src += advance;
      dst += advance;
      break;
#endif
    }
#endif

    // Tail (fewer than 16 bytes) or no SIMD: copy until the next '%'
    while (src != src_end && *src != '%') *dst++ = *src++;
    if (src == src_end) return dst;

    // *src == '%': decode consecutive escape sequences directly, without
    // going through the SIMD search for every '%'.  (Consecutive escapes are
    // common, e.g. percent-encoded UTF-8 as in Chinese URLs.)
    for (;;) {
      bool decoded = false;
      if (src_end - src >= 3) {
        std::optional<unsigned> byte = convert_2xdigit(src + 1);
        if (byte) {
          *dst++ = static_cast<char>(*byte);
          src += 3;
          decoded = true;
        }
      }
      if (!decoded) {
        // Invalid or truncated escape sequence: copy '%' verbatim
        *dst++ = '%';
        ++src;
      }
      if (src == src_end) return dst;
      // Consecutive escapes: keep decoding without the SIMD round trip
      if (*src == '%') continue;
      // Copy the separator character and start a new SIMD round
      *dst++ = *src++;
      break;
    }
  }

  return dst;
}

}  // namespace cbu
