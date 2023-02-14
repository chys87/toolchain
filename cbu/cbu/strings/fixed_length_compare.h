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

#include <array>
#include <concepts>
#include <cstdint>
#include <utility>

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

template <std::size_t N, typename T>
inline Type<N> Pick(const T* p) {
  Type<N> r;
  __builtin_memcpy(&r, p, N);
  return r;
}

}  // namespace is_all_zero_detail

template <std::size_t N, std::integral T>
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
    } else if constexpr (N > 96) {
      __m512i u = *(const __m512i_u*)p;
      for (std::size_t i = 64; i < N - 64; i += 64)
        u |= *(const __m512i_u*)(p + i);
      u |= *(const __m512i_u*)(p + N - 64);
      return _mm512_cmpneq_epi32_mask(u, _mm512_setzero_si512()) == 0;
#endif
#ifdef __AVX2__
    } else if constexpr (N == 32) {
      __m256i u = *(const __m256i_u*)p;
      return _mm256_testz_si256(u, u);
    } else if constexpr (N > 48) {
      __m256i u = *(const __m256i_u*)p;
      for (std::size_t i = 32; i < N - 32; i += 32)
        u |= *(const __m256i_u*)(p + i);
      u |= *(const __m256i_u*)(p + N - 32);
      return _mm256_testz_si256(u, u);
#endif
#ifdef __SSE4_1__
    } else if constexpr (N == 16) {
      __m128i u = *(const __m128i_u*)p;
      return _mm_testz_si128(u, u);
    } else if constexpr (N > 24) {
      __m128i u = *(const __m128i_u*)p;
      for (std::size_t i = 16; i < N - 16; i += 16)
        u |= *(const __m128i_u*)(p + i);
      u |= *(const __m128i_u*)(p + N - 16);
      return _mm_testz_si128(u, u);
#endif
    } else {
      constexpr std::size_t UZ = [&]() constexpr noexcept {
        std::size_t k = 1;
        while (k * 2 <= sizeof(std::uint64_t) && k * 2 <= N) k *= 2;
        return k;
      }();
      using UT = Type<UZ>;
      UT v = 0;
      for (std::size_t i = 0; i + UZ < N; i += UZ) v |= Pick<UZ>(p + i);
      v |= Pick<UZ>(p + N - UZ);
      return v == 0;
    }
  }
  for (std::size_t i = 0; i < N; ++i) {
    if (p[i] != 0) return false;
  }
  return true;
  }

template <std::size_t N, std::integral T>
inline constexpr bool IsAllZero(const T (&array)[N]) {
  return IsAllZero<N, T>(static_cast<const T*>(array));
}

template <std::size_t N, std::integral T>
inline constexpr bool IsAllZero(const std::array<T, N>& array) {
  return IsAllZero<N, T>(array.data());
}

}  // namespace cbu
