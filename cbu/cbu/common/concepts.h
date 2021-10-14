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

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace cbu {

template <typename T, typename C = char>
concept String_view_compat = requires (const T &t) {
  { t.data() } -> std::convertible_to<const C*>;
  { t.length() } -> std::convertible_to<std::size_t>;
};

template <typename T, typename C = char>
concept Zstring_view_compat = requires (const T &t) {
  { t.c_str() } -> std::convertible_to<const C*>;
  { t.length() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept Pointer = std::is_pointer_v<T>;

template <typename T>
concept No_cv = std::is_same_v<std::remove_cv_t<T>, T>;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Raw_arithmetic = Arithmetic<T> && No_cv<T>;

template <typename T>
concept Floating_point = std::is_floating_point_v<T>;

template <typename T>
concept Raw_floating_point = std::is_floating_point_v<T> && No_cv<T>;

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept Raw_integral = Integral<T> && No_cv<T>;

template <typename T>
concept Unsigned_integral = Integral<T> && std::is_unsigned_v<T>;

template <typename T>
concept Raw_unsigned_integral = Unsigned_integral<T> && No_cv<T>;

template <typename T>
concept Signed_integral = Integral<T> && std::is_signed_v<T>;

template <typename T>
concept Raw_signed_integral = Signed_integral<T> && No_cv<T>;

template <typename T>
concept Char_type = (Integral<T> && sizeof(T) == 1 &&
                     !std::is_same_v<std::remove_cv_t<T>, bool>);

template <typename T>
concept Raw_char_type = Char_type<T> && No_cv<T> && !std::is_array_v<T>;

template <typename T>
concept Trivial_type = std::is_trivial_v<T>;

template <typename T>
concept Raw_trivial_type =
    Trivial_type<T> and No_cv<T> and not std::is_array_v<T>;

template <typename T, typename U>
concept EqualityComparable = requires (T a, U b) {
  { a == b } -> std::convertible_to<bool>;
  { a != b } -> std::convertible_to<bool>;
};

template <typename T>
concept Std_string_char = std::is_same_v<T, char> ||
  std::is_same_v<T, wchar_t> ||
  std::is_same_v<T, char16_t> ||
  std::is_same_v<T, char32_t> ||
#if defined __cpp_char8_t && __cpp_char8_t >= 201811
  std::is_same_v<T, char8_t>
#else
  false
#endif
  ;

} // namespace cbu
