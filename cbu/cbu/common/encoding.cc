/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

#include "cbu/common/encoding.h"

#if __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif

#include "cbu/common/arch.h"
#include "cbu/common/bit.h"
#include "cbu/common/faststr.h"

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(unlikely)
#define CBU_UNLIKELY [[unlikely]]
#else
#define CBU_UNLIKELY
#endif

namespace cbu {

char8_t* char32_to_utf8(char8_t* w, char32_t u) noexcept {
  if (u < 0x800) {
    if (u < 0x80) {
      *w++ = u;
    } else {
#if defined __BMI2__
      w = memdrop_be<uint16_t>(w, _pdep_u32(u, 0x1f3f) | 0xc080u);
#else
      *w++ = (u >> 6) + 0xc0u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    }
  } else if (u < 0x10000) {
    if (is_in_utf16_surrogate_range(u)) CBU_UNLIKELY {
      // UTF-16 surrogate pairs
      return nullptr;
    } else {
      // Most Chinese characters fall here
#if defined __BMI2__
      memdrop_be<uint32_t>(w, _pdep_u32(u, 0x0f3f3f00) | 0xe0808000);
      w += 3;
#else
      *w++ = (u >> 12) + 0xe0u;
      *w++ = ((u >> 6) & 0x3fu) + 0x80u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    }
  } else CBU_UNLIKELY {
    if (u < 0x110000) {
#if defined __BMI2__
      w = memdrop_be<uint32_t>(w, _pdep_u32(u, 0x073f3f3f) | 0xf0808080u);
#else
      *w++ = (u >> 18) + 0xf0u;
      *w++ = ((u >> 12) & 0x3fu) + 0x80u;
      *w++ = ((u >> 6) & 0x3fu) + 0x80u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    } else {
      return nullptr;
    }
  }
  return w;
}

char* char32_to_utf8(char* dst, char32_t c) noexcept {
  return reinterpret_cast<char*>(
      char32_to_utf8(reinterpret_cast<char8_t*>(dst), c));
}

namespace {

bool validate_utf8_fallback(const void* ptr, const void* endptr) noexcept {
  const uint8_t* s = static_cast<const uint8_t*>(ptr);
  const uint8_t* end = static_cast<const uint8_t*>(endptr);
  while (s < end) {
    // Skip ASCII quickly
    if (s + 4 <= end) {
      while (true) {
        uint32_t v_native = mempick<uint32_t>(s) & 0x80808080;
        if (v_native == 0) {
          s += 4;
          if (s + 4 > end) {
            if (s >= end) return true;
            break;
          }
        } else {
          s += ctz(bswap_le(v_native)) / 8;
          break;
        }
      }
    }
    // Slow path
    uint8_t c = *s++;
    if ((c & 0x80) == 0) continue;
    if (c < 0xc2 || c > 0xf4) return false;
    unsigned xl = cbu::utf8_leading_byte_to_trailing_length(c);
    if (s + xl > end) return false;

    if (!validate_utf8_two_bytes(c, *s)) return false;

    if (xl >= 2) {
      uint16_t last_two_bytes = mempick<uint16_t>(s + xl - 2);
      if ((last_two_bytes & 0xc0c0) != 0x8080) return false;
    }
    s += xl;
  }
  return true;
}

#if defined __AVX2__ && defined __BMI__
bool validate_utf8_avx2_bmi(const void* ptr, const void* endptr) noexcept {
  const uint8_t* s = static_cast<const uint8_t*>(ptr);
  const uint8_t* end = static_cast<const uint8_t*>(endptr);

  while (s + 32 <= end) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s));
    // Quickly skip if the chunk is all ASCII
    if (_mm256_testz_si256(v, _mm256_set1_epi8(0x80))) {
      s += 32;
      continue;
    }

    // leading 1: [0xc2, 0xdf]
    __m256i vmask1 = in_range_epi8(v, 0xc2, 0xdf);
    uint32_t mask1 = _mm256_movemask_epi8(vmask1);
    // leading 2: [0xe0, 0xef]
    __m256i vmask2 = __m256i((__v32qi(v) & int8_t(0xf0)) == int8_t(0xe0));
    uint32_t mask2 = _mm256_movemask_epi8(vmask2);
    // leading 3: [0xf0, 0xf4]
    __m256i vmask3 = in_range_epi8(v, 0xf0, 0xf4);
    uint32_t mask3 = _mm256_movemask_epi8(vmask3);
    // following: [0x80, 0xbf]
    __m256i vmaskf = in_range_epi8(v, 0x80, 0xbf);
    uint32_t maskf = _mm256_movemask_epi8(vmaskf);

    // Is there any byte that's impossible in a legal UTF-8 string?
    if (!_mm256_testz_si256((vmask1 | vmask2 | vmask3 | vmaskf) ^ v,
                            _mm256_set1_epi8(0x80)))
      return false;

    if (uint32_t(mask1 * 0b10 | mask2 * 0b110 | mask3 * 0b1110) != maskf)
      return false;

    // mask1: Nothing to take special care of

    // mask2:
    // E0       A0..BF (excluding 80..9F)
    // ED       80..9F (excluding A0..BF)
    uint32_t e0 = _mm256_movemask_epi8(__m256i(__v32qi(v) == int8_t(0xe0)));
    uint32_t ed = _mm256_movemask_epi8(__m256i(__v32qi(v) == int8_t(0xed)));
    uint32_t x80_9f = _mm256_movemask_epi8(in_range_epi8(v, 0x80, 0x9f));
    uint32_t a0_bf = maskf ^ x80_9f;
    if (((e0 << 1) & x80_9f) | ((ed << 1) & a0_bf)) return false;

    // mask3:
    // F0       90..BF (excluding 80..8F)
    // F4       80..8F (excluding 90..BF)
    uint32_t f0 = _mm256_movemask_epi8(__m256i(__v32qi(v) == int8_t(0xf0)));
    uint32_t f4 = _mm256_movemask_epi8(__m256i(__v32qi(v) == int8_t(0xf4)));
    uint32_t x80_8f = _mm256_movemask_epi8(in_range_epi8(v, 0x80, 0x8f));
    uint32_t x90_bf = maskf ^ x80_8f;
    if (((f0 << 1) & x80_8f) | ((f4 << 1) & (x90_bf))) return false;

    // Now let's see by how many bytes we can advance.
    // Normally we can advance by 32 bytes; but if a character spans the end
    // of the 32-byte chunk, the bytes in the next 32-byte chunk haven't been
    // examined, and we simplify advance by 1/2/3 fewer bytes, so that they
    // will be verified in the next iteration.
    // Here we must use _tzcnt_u32 instead of ctz to ensure that it returns 32
    // (instead of unspecified) if the argument is 0.
    s += _tzcnt_u32((mask1 & 0x80000000) | (mask2 & 0xc0000000) |
                    (mask3 & 0xe0000000));
  }

  return validate_utf8_fallback(s, end);
}
#endif

}  // namespace

bool validate_utf8(const void* ptr, size_t size) noexcept {
#if defined __AVX2__ && defined __BMI__
  return validate_utf8_avx2_bmi(ptr, static_cast<const char*>(ptr) + size);
#else
  return validate_utf8_fallback(ptr, static_cast<const char*>(ptr) + size);
#endif
}

} // namespace cbu
