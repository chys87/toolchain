/*
 * cbu - chys's basic utilities
 * Copyright (c) 2023, chys <admin@CHYS.INFO>
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

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#ifdef __ARM_NEON
#  include <arm_neon.h>
#endif

#include <array>
#include <concepts>
#include <cstdint>

namespace cbu {
namespace is_all_zero_detail {

template <std::size_t N>
struct TypeImpl;

template <>
struct TypeImpl<1> {
  using type = std::uint8_t;
};

template <>
struct TypeImpl<2> {
  using type = std::uint16_t;
};

template <>
struct TypeImpl<4> {
  using type = std::uint32_t;
};

template <>
struct TypeImpl<8> {
  using type = std::uint64_t;
};

template <std::size_t N>
using Type = typename TypeImpl<N>::type;

template <std::size_t N>
using MinType = std::conditional_t<
    N <= 2, std::conditional_t<N <= 1, std::uint8_t, std::uint16_t>,
    std::conditional_t<N <= 4, std::uint32_t, std::uint64_t>>;

template <std::size_t N>
inline Type<N> Pick(const void* p) {
  Type<N> r;
  __builtin_memcpy(&r, p, N);
  return r;
}

template <std::size_t N>
inline MinType<N> PickToLeft(const void* p) {
  return Pick<sizeof(MinType<N>)>(p);
}

template <std::size_t N>
inline MinType<N> PickToRight(const void* p) {
  return Pick<sizeof(MinType<N>)>(static_cast<const char*>(p) -
                                  sizeof(MinType<N>));
}

#ifdef __SSE4_1__
template <std::size_t N>
inline __m128i Pick128ToLeft(const void* p) {
  if constexpr (N <= 4)
    return _mm_cvtsi32_si128(Pick<4>(p));
  else if constexpr (N <= 8)
    return _mm_cvtsi64_si128(Pick<8>(p));
  else
    return *(const __m128i_u*)p;
}

template <std::size_t N>
inline __m128i Pick128ToRight(const void* p) {
  if constexpr (N <= 4)
    return _mm_cvtsi32_si128(Pick<4>(static_cast<const char*>(p) - 4));
  else if constexpr (N <= 8)
    return _mm_cvtsi64_si128(Pick<8>(static_cast<const char*>(p) - 8));
  else
    return *(const __m128i_u*)(static_cast<const char*>(p) - 16);
}
#endif

#ifdef __AVX2__
template <std::size_t N>
inline __m256i Pick256ToLeft(const void* p) {
  if constexpr (N <= 16)
    return _mm256_zextsi128_si256(Pick128ToLeft<N>(p));
  else
    return *(const __m256i_u*)p;
}

template <std::size_t N>
inline __m256i Pick256ToRight(const void* p) {
  if constexpr (N <= 16)
    return _mm256_zextsi128_si256(Pick128ToRight<N>(p));
  else
    return *(const __m256i_u*)(static_cast<const char*>(p) - 32);
}
#endif

#ifdef __AVX512F__
template <std::size_t N>
inline __m512i Pick512ToLeft(const void* p) {
  if constexpr (N <= 16)
    return _mm512_zextsi128_si512(Pick128ToLeft<N>(p));
  else if constexpr (N <= 32)
    return _mm512_zextsi256_si512(Pick256ToLeft<N>(p));
  else
    return *(const __m512i_u*)p;
}

template <std::size_t N>
inline __m512i Pick512ToRight(const void* p) {
  if constexpr (N <= 16)
    return _mm512_zextsi128_si512(Pick128ToRight<N>(p));
  else if constexpr (N <= 32)
    return _mm512_zextsi256_si512(Pick256ToRight<N>(p));
  else
    return *(const __m512i_u*)(static_cast<const char*>(p) - 64);
}
#endif

}  // namespace is_all_zero_detail

struct IsAllZeroOptions {
  // Set it to true ifthe right side is likely aligned.
  // This option is provided because IsAllZero is usually used to
  // check for reserved fields in a structure, which are likely
  // right aligned.
  bool right_align = false;
};

template <std::size_t N, IsAllZeroOptions Opts = IsAllZeroOptions{},
          std::integral T>
inline constexpr bool IsAllZero(const T* p) {
  using namespace is_all_zero_detail;

  if !consteval {
    if constexpr (N == 0) {
      return true;
    } else if constexpr (sizeof(T) > 1) {
      return IsAllZero<N * sizeof(T), char>(reinterpret_cast<const char*>(p));
    } else if constexpr (N == 1) {
      return (*p == 0);
    } else if constexpr (N == 2 || N == 4 || N == 8) {
      return Pick<N>(p) == 0;
#ifdef __AVX512F__
    } else if constexpr (N == 64) {
      __m512i u = *(const __m512i_u*)p;
      return _mm512_cmpneq_epi32_mask(u, _mm512_setzero_si512()) == 0;
    } else if constexpr (N > 64) {
      __m512i u = _mm512_setzero_si512();
      if constexpr (Opts.right_align) {
        if constexpr (N % 64) u = Pick512ToLeft<N % 64>(p);
        for (std::size_t i = N % 64; i < N; i += 64)
          u |= *(const __m512i_u*)(p + i);
      } else {
        if constexpr (N % 64) u = Pick512ToRight<N % 64>(p + N);
        for (std::size_t i = N / 64 * 64; i; i -= 64)
          u |= *(const __m512i_u*)(p + i - 64);
      }
      return _mm512_cmpneq_epi32_mask(u, _mm512_setzero_si512()) == 0;
#endif
#ifdef __AVX2__
    } else if constexpr (N == 32) {
      __m256i u = *(const __m256i_u*)p;
      return _mm256_testz_si256(u, u);
    } else if constexpr (N > 32) {
      __m256i u = _mm256_setzero_si256();
      if constexpr (Opts.right_align) {
        if constexpr (N % 32) u = Pick256ToLeft<N % 32>(p);
        for (std::size_t i = N % 32; i < N; i += 32)
          u |= *(const __m256i_u*)(p + i);
      } else {
        if constexpr (N % 32) u = Pick256ToRight<N % 32>(p + N);
        for (std::size_t i = N / 32 * 32; i; i -= 32)
          u |= *(const __m256i_u*)(p + i - 32);
      }
      return _mm256_testz_si256(u, u);
#endif
#ifdef __SSE4_1__
    } else if constexpr (N == 16) {
      __m128i u = *(const __m128i_u*)p;
      return _mm_testz_si128(u, u);
    } else if constexpr (N > 24) {
      __m128i u = _mm_setzero_si128();
      if constexpr (Opts.right_align) {
        if constexpr (N % 16) u = Pick128ToLeft<N % 16>(p);
        for (std::size_t i = N % 16; i < N; i += 16)
          u |= *(const __m128i_u*)(p + i);
      } else {
        if constexpr (N % 16) u = Pick128ToRight<N % 16>(p + N);
        for (std::size_t i = N / 16 * 16; i; i -= 16)
          u |= *(const __m128i_u*)(p + i - 16);
      }
      return _mm_testz_si128(u, u);
#endif
#ifdef __ARM_NEON
    } else if constexpr (N == 16) {
      return (Pick<8>(p) | Pick<8>(p + 8)) == 0;
    } else if constexpr (N > 24) {
      uint8x16_t u = {};
      if constexpr (Opts.right_align) {
        if constexpr (N % 16) u = *(const uint8x16_t*)p;
        for (std::size_t i = N % 16; i < N; i += 16)
          u |= *(const uint8x16_t*)(p + i);
      } else {
        if constexpr (N % 16) u = *(const uint8x16_t*)(p + N - 16);
        for (std::size_t i = N / 16 * 16; i; i -= 16)
          u |= *(const uint8x16_t*)(p + i - 16);
      }
      u = vpmaxq_u8(u, u);
      return vgetq_lane_u64(vreinterpretq_u64_u8(u), 0) == 0;
#endif
    } else {
      constexpr std::size_t UZ = [&]() constexpr noexcept {
        std::size_t k = 1;
        while (k * 2 <= sizeof(std::uint64_t) && k * 2 <= N) k *= 2;
        return k;
      }();
      using UT = Type<UZ>;
      UT v = 0;
      if constexpr (Opts.right_align) {
        if constexpr (N % UZ) v |= PickToLeft<N % UZ>(p);
        for (std::size_t i = N % UZ; i < N; i += UZ) v |= Pick<UZ>(p + i);
      } else {
        if constexpr (N % UZ) v = PickToRight<N % UZ>(p + N);
        for (std::size_t i = N / UZ * UZ; i; i -= UZ) v |= Pick<UZ>(p + i - UZ);
      }
      return v == 0;
    }
  }
  for (std::size_t i = 0; i < N; ++i) {
    if (p[i] != 0) return false;
  }
  return true;
}

template <IsAllZeroOptions Opts = IsAllZeroOptions{}, std::size_t N,
          std::integral T>
inline constexpr bool IsAllZero(const T (&array)[N]) {
  return IsAllZero<N, Opts, T>(static_cast<const T*>(array));
}

template <IsAllZeroOptions Opts = IsAllZeroOptions{}, std::size_t N,
          std::integral T>
inline constexpr bool IsAllZero(const std::array<T, N>& array) {
  return IsAllZero<N, Opts, T>(array.data());
}

}  // namespace cbu
