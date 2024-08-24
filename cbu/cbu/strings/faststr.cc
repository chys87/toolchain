/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include "cbu/strings/faststr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <bit>

#include "cbu/common/stdhack.h"
#include "cbu/compat/string.h"
#include "cbu/math/common.h"
#include "cbu/math/strict_overflow.h"

namespace cbu {

alignas(64) const char arch_linear_bytes64[64] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

void* memdrop_var32(void* dst, uint32_t v, size_t n) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  _mm_mask_storeu_epi8(dst, (1 << n) - 1, _mm_cvtsi32_si128(v));
  return static_cast<char*>(dst) + n;
#endif

  if (std::endian::native == std::endian::little) {

    char *d = static_cast<char *>(dst);
    char *e = d + n;

    switch (n) {
      default:
        __builtin_unreachable();
      case 4:
        memdrop4(d, v);
        break;
      case 3:
        memdrop2(d, v);
        d[2] = uint32_t(v) >> 16;
        break;
      case 2:
        memdrop2(d, v);
        break;
      case 1:
        d[0] = v;
        break;
      case 0:
        break;
    }
    return e;

  } else {
    return compat::mempcpy(dst, &v, n);
  }
}

void* memdrop_var64(void* dst, uint64_t v, size_t n) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  _mm_mask_storeu_epi8(dst, (1 << n) - 1, _mm_cvtsi64_si128(v));
  return static_cast<char*>(dst) + n;
#endif

  if (std::endian::native == std::endian::little) {

    char *d = static_cast<char *>(dst);
    char *e = d + n;

    switch (n) {
      default:
        __builtin_unreachable();
      case 8:
        memdrop8(d, v);
        break;
      case 5:
        memdrop4(d, uint32_t(v));
        d[4] = uint8_t(v >> 32);
        break;
      case 6:
        memdrop4(d, uint32_t(v));
        memdrop2(d + 4, uint16_t(v >> 32));
        break;
      case 7:
        memdrop4(d, uint32_t(v));
        memdrop4(d + 3, uint32_t(v >> 24));
        break;
      case 4:
        memdrop4(d, v);
        break;
      case 3:
        memdrop2(d, v);
        d[2] = uint32_t(v) >> 16;
        break;
      case 2:
        memdrop2(d, v);
        break;
      case 1:
        d[0] = v;
        break;
      case 0:
        break;
    }
    return e;

  } else {
    return compat::mempcpy(dst, &v, n);
  }
}

#if __WORDSIZE >= 64
void* memdrop_var128(void* dst, unsigned __int128 v, std::size_t n) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  _mm_mask_storeu_epi8(dst, (1 << n) - 1, _mm_set_epi64x(v >> 64, v));
  return static_cast<char*>(dst) + n;
#endif

  if (std::endian::native == std::endian::little) {
    if (n <= 8) {
      return memdrop_var64(dst, uint64_t(v), n);
    } else {
      memdrop8(static_cast<char*>(dst), uint64_t(v));
      memdrop8(static_cast<char*>(dst) + n - 8,
               uint64_t(v >> ((n - 8) * 8 % 64)));
      return static_cast<char*>(dst) + n;
    }
  } else {
    return compat::mempcpy(dst, &v, n);
  }
}
#endif

#ifdef __SSE2__
void* memdrop(void* dst, __m128i v, std::size_t n) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  _mm_mask_storeu_epi8(dst, (1 << n) - 1, v);
  return static_cast<char*>(dst) + n;
#endif
  char *d = static_cast<char *>(dst);
  char *e = d + n;

  if (n > 8) {
    __m128i u = _mm_shuffle_epi8(
        v, *(const __m128i_u *)(arch_linear_bytes64 + n - 8));
    memdrop8(d, __v2di(v)[0]);
    memdrop8(e - 8, __v2di(u)[0]);
    return e;
  } else {
    return memdrop_var64(d, __v2di(v)[0], n);
  }
}
#endif

#ifdef __AVX2__
void* memdrop(void* dst, __m256i v, std::size_t n) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  // TODO: We can do better by using 32-bit mask and forcing SHLX instead of SHL
  _mm256_mask_storeu_epi8(dst, (1ull << n) - 1, v);
  return static_cast<char*>(dst) + n;
#endif
  char *d = static_cast<char *>(dst);

  if (n > 16) {
    *(__m128i_u *)d = _mm256_extracti128_si256(v, 0);
    __m128i vv = _mm256_extracti128_si256(v, 1);
    return memdrop(d + 16, vv, n - 16);
  } else {
    return memdrop(d, _mm256_extracti128_si256(v, 0), n);
  }
}
#endif

}  // namespace cbu
