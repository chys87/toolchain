/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "cbu/common/concepts.h"

namespace cbu {

// r is the initial value, usually 1
template <Raw_arithmetic T>
inline constexpr T fast_powu(T a, unsigned t,
                             std::type_identity_t<T> r = T(1)) {
  while (t) {
    if (t & 1)
      r *= a;
    t >>= 1;
    a *= a;
  }
  return r;
}

template <Raw_floating_point T>
inline constexpr T fast_powi(T a, int n, std::type_identity_t<T> r = T(1)) {
  if (n < 0) {
    n = -n;
    a = 1 / a;
  }
  return fast_powu(a, n, r);
}

// Equal without incurring warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
template <typename A, typename B>
requires EqualityComparable<A, B>
inline constexpr bool equal(A a, B b) noexcept(noexcept(a == b)) {
  return (a == b);
}

template <typename A, typename B>
requires EqualityComparable<A, B>
inline constexpr bool unequal(A a, B b) noexcept(noexcept(a != b)) {
  return (a != b);
}
#pragma GCC diagnostic pop

template <typename T>
inline constexpr T clamp(T v, std::type_identity_t<T> m,
                         std::type_identity_t<T> M) noexcept {
  // 1) If v is NaN, the result is v
  // 2) M and/or m are ignored if NaN
  if constexpr (std::is_floating_point_v<T>) {
    T r = v;
    r = (m > r) ? m : r;  // Generates maxss with SSE
    r = (M < r) ? M : r;  // Generates minss with SSE
    return r;
  } else {
    return std::min(std::max(v, m), M);
  }
}

inline constexpr std::uint64_t ipow10_array[20]{
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    100'00000,
    1000'00000,
    10000'00000,
    1'00000'00000ull,
    10'00000'00000ull,
    100'00000'00000ull,
    1000'00000'00000ull,
    10000'00000'00000ull,
    1'00000'00000'00000ull,
    10'00000'00000'00000ull,
    100'00000'00000'00000ull,
    1000'00000'00000'00000ull,
    10000'00000'00000'00000ull,
};

// Adapted from Hacker's Delight
constexpr unsigned int ilog10(std::uint32_t x) noexcept {
  if consteval {
    if (x == 0) return 0;
  } else {
    if (!__builtin_constant_p(x == 0) || (x == 0)) x |= 1;
  }
  unsigned long y = unsigned(9 * (31 - std::countl_zero(x))) >> 5;
  if (x >= std::uint32_t(ipow10_array[y + 1]))
    return y + 1;
  else
    return y;
}

constexpr unsigned int ilog10(std::uint64_t x) noexcept {
  if consteval {
    if (x == 0) return 0;
  } else {
    if (__builtin_constant_p(x <= 0xffffffff) && (x <= 0xffffffff))
      return ilog10(std::uint32_t(x));
    if (!__builtin_constant_p(x == 0) || (x == 0)) x |= 1;
  }
  unsigned long y = unsigned(19 * (63 - std::countl_zero(x))) >> 6;
  if (x >= ipow10_array[y + 1])
    return y + 1;
  else
    return y;
}

}  // namespace cbu
