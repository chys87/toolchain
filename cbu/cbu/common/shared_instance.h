/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include <concepts>
#include <limits>
#include <type_traits>

namespace cbu {

namespace detail {

template <typename T, auto... args>
struct SharedConst {
  static inline constexpr T value{args...};
};

template <typename T, auto... args>
struct SharedConst<const T, args...> : SharedConst<T, args...> {};

alignas(8) inline const char shared_const_zero[8]{};

template <typename T>
  requires(std::is_same_v<std::remove_cvref_t<T>, T> and sizeof(T) <= 8 and
           (std::is_integral_v<T> or (std::is_floating_point_v<T> and
                                      std::numeric_limits<T>::is_iec559)))
struct SharedConst<T> {
  static inline const T& value = reinterpret_cast<const T&>(shared_const_zero);
};

template <typename T, auto init_value>
  requires(std::is_same_v<std::remove_cvref_t<T>, T> and sizeof(T) <= 8 and
           (std::is_integral_v<T> or (std::is_floating_point_v<T> and
                                      std::numeric_limits<T>::is_iec559)) and
           std::is_integral_v<decltype(init_value)> and init_value == 0)
struct SharedConst<T, init_value> : SharedConst<T> {};

}  // namespace detail

template <typename T, auto... args>
inline T shared(args...);

template <typename T, auto... args>
inline constinit T shared_constinit(args...);

template <typename T, auto... args>
inline constexpr T shared_constexpr(args...);

template <typename T, auto... args>
inline const T& shared_const = detail::SharedConst<T, args...>::value;

template <auto value>
inline const auto& as_shared =
    shared_const<std::remove_cvref_t<decltype(value)>, value>;

} // namespace cbu
