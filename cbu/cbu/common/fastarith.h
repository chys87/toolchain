/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

#include "bit_cast.h"
#include "concepts.h"
#include "cbu/compat/type_identity.h"
#include <cstdint>

namespace cbu {
inline namespace cbu_fastarith {

// r is the initial value, usually 1
template <Raw_arithmetic T>
inline constexpr T fast_powu(T a, unsigned t,
                             compat::type_identity_t<T> r = T(1)) {
  while (t) {
    if (t & 1)
      r *= a;
    t >>= 1;
    a *= a;
  }
  return r;
}

template <Raw_floating_point T>
inline constexpr T fast_powi(T a, int n,
                             compat::type_identity_t<T> r = T(1)) {
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

template <Raw_floating_point T>
inline constexpr T clamp(T v,
                         compat::type_identity_t<T> m,
                         compat::type_identity_t<T> M) noexcept {
  // 1) If v is NaN, the result is v
  // 2) M and/or m are ignored if NaN
  T r = v;
  r = (m > r) ? m : r; // Generates maxss with SSE
  r = (M < r) ? M : r; // Generates minss with SSE
  return r;
}

namespace fastarith_detail {

#ifndef __has_attribute
# define __has_attribute(...) 0
#endif
#if __has_attribute(__error__)
void error() __attribute__((__error__("Unreachable code")));
#else
[[noreturn]] void error();
#endif

} // namespace fastarith_detail

inline std::uint32_t float_to_uint32(float x) {
  if constexpr (sizeof(float) == 4) {
    return bit_cast<std::uint32_t>(x);
  } else {
    fastarith_detail::error();
  }
}

inline float uint32_to_float(std::uint32_t u) {
  if constexpr (sizeof(float) == 4) {
    return bit_cast<float>(u);
  } else {
    fastarith_detail::error();
  }
}

inline std::uint64_t double_to_uint64(double x) {
  if constexpr (sizeof(double) == 8) {
    return bit_cast<std::uint64_t>(x);
  } else {
    fastarith_detail::error();
  }
}

inline double uint64_to_double(std::uint64_t u) {
  if constexpr (sizeof(double) == 8) {
    return bit_cast<double>(u);
  } else {
    fastarith_detail::error();
  }
}

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline bool mul_overflow(A a, B b, C *c) noexcept {
  return __builtin_mul_overflow(a, b, c);
}

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline bool add_overflow(A a, B b, C *c) noexcept {
  return __builtin_add_overflow(a, b, c);
}

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline bool sub_overflow(A a, B b, C *c) noexcept {
  return __builtin_sub_overflow(a, b, c);
}

 // GCC's builtin generates bad code
#if defined __GCC_ASM_FLAG_OUTPUTS__ && \
    (defined __i386__ || defined __x86_64__)
template <Raw_unsigned_integral U, Raw_integral V>
requires (4 <= sizeof(U) and sizeof(V) <= sizeof(U))
inline bool sub_overflow(U a, V b, U *c) noexcept {
  if (std::is_signed<V>::value && (!__builtin_constant_p(b) || b < 0))
    return __builtin_sub_overflow(a, b, c);
  bool res;
  asm ("sub %[b], %[a]" : "=@ccc"(res), "=r"(*c) : [a]"1"(a), [b]"g"(b));
  return res;
}
#endif // __GCC_ASM_FLAG_OUTPUTS__ && (__i386__ || __x86_64__)

template <Integral U, Integral V>
inline bool sub_overflow(U *a, V b) noexcept {
	return sub_overflow(*a, b, a);
}

// Map uint32_t or uint64_t to floating point values in [0, 1)
float map_uint32_to_float(std::uint32_t v) noexcept;
double map_uint64_to_double(std::uint64_t v) noexcept;


constexpr std::uint32_t ipow10_array[10] = {
  1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

unsigned int ilog10(std::uint32_t) noexcept __attribute__((__const__));


} // inline namespace cbu_fastarith
} // namespace cbu
