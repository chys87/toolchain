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

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "cbu/common/concepts.h"

namespace cbu {

namespace detail {

template <typename T, auto... args>
struct SharedConst {
  // Use explicit alignas to stop GCC from increasing alignment of array types
  alignas(T) static inline constexpr T value{args...};
};

template <typename T, auto... args>
struct SharedConst<const T, args...> : SharedConst<T, args...> {};

template <std::integral T, auto... args>
  requires(sizeof...(args) != 1)
struct SharedConst<T, args...> : SharedConst<T, T{args...}> {};

template <std::integral T, auto arg>
  requires(!std::is_same_v<std::remove_cvref_t<T>,
                           std::remove_cvref_t<decltype(arg)>>)
struct SharedConst<T, arg> : SharedConst<T, T{arg}> {};

template <Raw_signed_integral T, auto... args>
struct SharedConst<T, args...> {
  static inline const T& value = reinterpret_cast<const T&>(
      SharedConst<std::make_unsigned_t<T>,
                  std::make_unsigned_t<T>(T(args...))>::value);
};

template <auto init_value>
  requires(std::is_integral_v<decltype(init_value)> && init_value == 0 &&
           std::numeric_limits<float>::is_iec559)
struct SharedConst<float, init_value> {
  static inline const float& value =
      reinterpret_cast<const float&>(SharedConst<std::uint32_t, 0>::value);
};

template <auto init_value>
  requires(std::is_integral_v<decltype(init_value)> && init_value == 0 &&
           std::numeric_limits<double>::is_iec559)
struct SharedConst<double, init_value> {
  static inline const double& value =
      reinterpret_cast<const double&>(SharedConst<std::uint64_t, 0>::value);
};

}  // namespace detail

template <typename T, auto... args>
inline T shared = T(args...);

template <typename T, auto... args>
inline constinit T shared_constinit = T(args...);

template <typename T, auto... args>
inline constexpr T shared_constexpr = T(args...);

template <typename T, auto... args>
inline const T& shared_const = detail::SharedConst<T, args...>::value;

template <auto value>
inline const auto& as_shared =
    shared_const<std::remove_cvref_t<decltype(value)>, value>;

} // namespace cbu
