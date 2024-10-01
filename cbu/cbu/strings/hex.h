/*
 * cbu - chys's basic utilities
 * Copyright (c) 2023-2024, chys <admin@CHYS.INFO>
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

#if defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  include <arm_neon.h>
#endif
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif

#include <bit>
#include <concepts>
#include <cstdint>

namespace cbu {

constexpr char* ByteToHex(char* dst, std::uint8_t x) noexcept {
  *dst++ = "0123456789abcdef"[x >> 4];
  *dst++ = "0123456789abcdef"[x & 15];
  return dst;
}

constexpr char* LittleEndian16ToHex(char* dst, std::uint16_t x) noexcept {
  dst = ByteToHex(dst, std::uint8_t(x));
  dst = ByteToHex(dst, std::uint8_t(x >> 8));
  return dst;
}

constexpr char* LittleEndian32ToHex(char* dst, std::uint32_t x) noexcept {
#if defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if !consteval {
    auto a = uint8x8_t(uint32x2_t{x >> 4, 0});
    auto b = uint8x8_t(uint32x2_t{x, 0});
    auto vx = vzip1_u8(a, b);
    *(uint8x8_t*)dst =
        vqtbl1_u8(*(const uint8x16_t*)"0123456789abcdef", vx & 0xf);
    return dst + 8;
  }
#elif defined __SSSE3__
  if !consteval {
    __m128i vx =
        _mm_unpacklo_epi8(_mm_cvtsi32_si128(x >> 4), _mm_cvtsi32_si128(x));
    *(uint64_t*)dst = _mm_cvtsi128_si64(_mm_shuffle_epi8(
        *(const __m128i_u*)"0123456789abcdef", vx & _mm_set1_epi8(0x0f)));
    return dst + 8;
  }
#endif
  for (int i = 0; i < 4; ++i) {
    dst = ByteToHex(dst, std::uint8_t(x));
    x >>= 8;
  }
  return dst;
}

constexpr char* LittleEndian64ToHex(char* dst, std::uint64_t x) noexcept {
#if defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if !consteval {
    auto a = uint8x16_t(uint64x2_t{x >> 4, 0});
    auto b = uint8x16_t(uint64x2_t{x, 0});
    auto vx = vzip1q_u8(a, b);
    *(uint8x16_t*)dst =
        vqtbl1q_u8(*(const uint8x16_t*)"0123456789abcdef", vx & 0xf);
    return dst + 16;
  }
#elif defined __SSSE3__
  if !consteval {
    __m128i vx =
        _mm_unpacklo_epi8(_mm_cvtsi64_si128(x >> 4), _mm_cvtsi64_si128(x));
    *(__m128i_u*)dst = _mm_shuffle_epi8(*(const __m128i_u*)"0123456789abcdef",
                                        vx & _mm_set1_epi8(0x0f));
    return dst + 16;
  }
#endif

  for (int i = 0; i < 8; ++i) {
    dst = ByteToHex(dst, std::uint8_t(x));
    x >>= 8;
  }
  return dst;
}

constexpr char* ToHex(char* dst, std::unsigned_integral auto x) noexcept {
  if !consteval {
    if constexpr (sizeof(x) == 8) {
      return LittleEndian64ToHex(dst, std::byteswap(x));
    } else if constexpr (sizeof(x) == 4) {
      return LittleEndian32ToHex(dst, std::byteswap(x));
    } else if constexpr (sizeof(x) == 2) {
      return LittleEndian16ToHex(dst, std::byteswap(x));
    } else if constexpr (sizeof(x) == 1) {
      return ByteToHex(dst, x);
    }
  }

  for (std::size_t i = 0; i < 2 * sizeof(x); ++i) {
    *dst++ = "0123456789abcdef"[(x >> ((2 * sizeof(x) - i - 1) * 4)) & 15];
  }
  return dst;
}

}  // namespace cbu
