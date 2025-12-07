/*
 * cbu - chys's basic utilities
 * Copyright (c) 2023-2025, chys <admin@CHYS.INFO>
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

#if defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  include <arm_neon.h>
#endif
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif

#include <bit>
#include <concepts>
#include <cstdint>
#include <optional>

#include "cbu/compat/compilers.h"
#include "cbu/strings/faststr.h"

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

namespace hex_detail {

template <bool assume_valid, typename T = unsigned>
using ConvertXDigit = std::conditional_t<assume_valid, T, std::optional<T>>;

template <std::unsigned_integral T>
constexpr T v(T x) noexcept {
  return x;
}

template <std::unsigned_integral T>
constexpr T v(const std::optional<T>& x) noexcept {
  return *x;
}

}  // namespace hex_detail

// Convert a hexadecimal digit to number
template <bool assume_valid = false>
CBU_AARCH64_PRESERVE_ALL constexpr hex_detail::ConvertXDigit<assume_valid>
convert_xdigit(std::uint8_t c) noexcept {
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  } else if constexpr (assume_valid) {
    return (((c | 0x20) - 'a') + 10);
  } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
    return (((c | 0x20) - 'a') + 10);
  } else {
    return std::nullopt;
  }
}

// Convert 2 hexadecimal digits
template <bool assume_valid = false>
CBU_AARCH64_PRESERVE_ALL constexpr hex_detail::ConvertXDigit<assume_valid>
convert_2xdigit(const char* s) noexcept {
  auto a = convert_xdigit<assume_valid>(*s++);
  if (!a) return a;
  auto b = convert_xdigit<assume_valid>(*s++);
  if (!b) return b;
  return *a * 16 + *b;
}

// Convert 4 hexadecimal digits
template <bool assume_valid = false>
CBU_AARCH64_PRESERVE_ALL constexpr hex_detail::ConvertXDigit<assume_valid>
convert_4xdigit(const char* s) noexcept {
#if defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(__v4su{mempick_be<uint32_t>(s), 0, 0, 0});
    __v16qu digits = __v16qu(v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = __v16qu(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (!_mm_testc_si128(__m128i(digits | alpha),
                           _mm_setr_epi32(-1, 0, 0, 0)))
        return std::nullopt;
    }
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    uint32_t t = __v4su(res)[0];
    return _pext_u32(t, 0x0f0f0f0f);
  }
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if !consteval {
    uint32_t vc = mempick_le<uint32_t>(s);
    uint8x8_t v = vrev32_u8(uint8x8_t(uint32x2_t{vc, 0}));
    uint8x8_t v_digits = uint8x8_t(v - '0');
    uint8x8_t digits = uint8x8_t(v_digits < 10);
    uint8x8_t v_alpha = (v | 0x20) - 'a';
    uint8x8_t alpha = uint8x8_t(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (uint32x2_t(digits | alpha)[0] != 0xffffffff) return std::nullopt;
    }
    uint8x8_t r = v_alpha + 10 + (digits & ('a' - '0' - 10));
    uint16x4_t t = (uint16x4_t(r) >> 4) | (uint16x4_t(r) & 0xf);
    uint8x8_t u = vuzp1_u8(uint8x8_t(t), uint8x8_t(t));
    return uint16x4_t(u)[0];
  }
#endif
  auto a = convert_xdigit<assume_valid>(*s++);
  if constexpr (!assume_valid)
    if (!a) return a;
  auto b = convert_xdigit<assume_valid>(*s++);
  if constexpr (!assume_valid)
    if (!b) return b;
  auto c = convert_xdigit<assume_valid>(*s++);
  if constexpr (!assume_valid)
    if (!c) return c;
  auto d = convert_xdigit<assume_valid>(*s++);
  if constexpr (!assume_valid)
    if (!d) return d;
  return ((hex_detail::v(a) * 16 + hex_detail::v(b)) * 16 + hex_detail::v(c)) *
             16 +
         hex_detail::v(d);
}

// Convert 8 hexadecimal digits
template <bool assume_valid = false>
CBU_AARCH64_PRESERVE_ALL constexpr hex_detail::ConvertXDigit<assume_valid>
convert_8xdigit(const char* s) noexcept {
#if defined __x86_64__ && defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(__v2du{mempick_be<uint64_t>(s), 0});
    __v16qu digits = __v16qu(v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = __v16qu(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (!_mm_testc_si128(__m128i(digits | alpha),
                           _mm_setr_epi32(-1, -1, 0, 0)))
        return std::nullopt;
    }
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    uint64_t t = __v2du(res)[0];
    return unsigned(_pext_u64(t, 0x0f0f0f0f'0f0f0f0f));
  }
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if !consteval {
    uint8x8_t v = vrev64_u8(uint8x8_t(uint64x1_t{mempick_le<uint64_t>(s)}));
    uint8x8_t v_digits = uint8x8_t(v - '0');
    uint8x8_t digits = uint8x8_t(v_digits < 10);
    uint8x8_t v_alpha = (v | 0x20) - 'a';
    uint8x8_t alpha = uint8x8_t(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (uint64x1_t(digits | alpha)[0] != uint64_t(-1)) return std::nullopt;
    }
    uint8x8_t r = v_alpha + 10 + (digits & ('a' - '0' - 10));
    uint16x4_t t = (uint16x4_t(r) >> 4) | (uint16x4_t(r) & 0xf);
    uint8x8_t u = vuzp1_u8(uint8x8_t(t), uint8x8_t(t));
    return uint32x2_t(u)[0];
  }
#endif
  auto a = convert_4xdigit<assume_valid>(s);
  auto b = convert_4xdigit<assume_valid>(s + 4);
  if constexpr (!assume_valid) {
    if (!a || !b) return std::nullopt;
  }
  return (hex_detail::v(a) << 16) + hex_detail::v(b);
}

// Convert 16 hexadecimal digits
template <bool assume_valid = false>
CBU_AARCH64_PRESERVE_ALL constexpr hex_detail::ConvertXDigit<assume_valid,
                                                             std::uint64_t>
convert_16xdigit(const char* s) noexcept {
#if defined __x86_64__ && defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(_mm_shuffle_epi8(
          *(const __m128i_u*)s,
          _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)));
    __v16qu digits = __v16qu(v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = __v16qu(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (!_mm_testc_si128(__m128i(digits | alpha), _mm_set1_epi8(-1)))
        return std::nullopt;
    }
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    std::uint64_t lo = _pext_u64(__v2du(res)[0], 0x0f0f0f0f'0f0f0f0f);
    std::uint64_t hi = _pext_u64(__v2du(res)[1], 0x0f0f0f0f'0f0f0f0f);
    return (hi << 32) | lo;
  }
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if !consteval {
    uint8x16_t v = vrev64q_u8(uint8x16_t(
        uint64x2_t{mempick_le<uint64_t>(s + 8), mempick_le<uint64_t>(s)}));
    uint8x16_t v_digits = uint8x16_t(v - '0');
    uint8x16_t digits = uint8x16_t(v_digits < 10);
    uint8x16_t v_alpha = (v | 0x20) - 'a';
    uint8x16_t alpha = uint8x16_t(v_alpha < 6);
    if constexpr (!assume_valid) {
      if (vminvq_u8(digits | alpha) != 0xff) return std::nullopt;
    }
    uint8x16_t r = v_alpha + 10 + (digits & ('a' - '0' - 10));
    uint16x8_t t = (uint16x8_t(r) >> 4) | (uint16x8_t(r) & 0xf);
    uint8x16_t u = vuzp1q_u8(uint8x16_t(t), uint8x16_t(t));
    return uint64x2_t(u)[0];
  }
#endif
  auto a = convert_8xdigit<assume_valid>(s);
  auto b = convert_8xdigit<assume_valid>(s + 8);
  if constexpr (!assume_valid) {
    if (!a || !b) return std::nullopt;
  }
  return (std::uint64_t(hex_detail::v(a)) << 32) | hex_detail::v(b);
}

}  // namespace cbu
