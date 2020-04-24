/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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

#include <type_traits>

namespace cbu {
inline namespace cbu_type_traits {

// And
template <typename...> struct and_;
template <> struct and_<> : std::true_type {};
template <typename... V>
struct and_<std::true_type, V...> : and_<V...>::type {};
template <typename... V>
struct and_<std::false_type, V...> : std::false_type {};
template <typename T, typename... V>
struct and_<T, V...> : and_<std::bool_constant<T::value>, V...>::type {};

template <typename... Traits>
inline constexpr bool and_v = and_<Traits...>::value;

// Or
template <typename...> struct or_;
template <> struct or_<> : std::false_type {};
template <typename... V>
struct or_<std::true_type, V...> : std::true_type {};
template <typename... V>
struct or_<std::false_type, V...> : or_<V...>::type {};
template <typename T, typename... V>
struct or_<T, V...> : or_<std::bool_constant<T::value>, V...>::type {};

template <typename... Traits>
inline constexpr bool or_v = or_<Traits...>::value;

// Not
template <typename T> struct not_ : std::bool_constant<!T::value> {};

template <typename T>
inline constexpr bool not_v = !T::value;

// None
template <typename... T> struct none_ : not_<or_<T...>>::type {};

template <typename... Traits>
inline constexpr bool none_v = none_<Traits...>::value;

// Check whether a type is one of many types
template <typename T, typename... C>
struct is_one_of : or_<std::is_same<T, C>...>::type {};

template <typename T, typename... C>
inline constexpr bool is_one_of_v = is_one_of<T, C...>::value;

// Extended is_same
template <typename...> struct is_same;
template <> struct is_same<> : std::true_type {};
template <typename U, typename... V>
struct is_same<U, V...> : and_<std::is_same<U, V>...>::type {};

template <typename... Types>
inline constexpr bool is_same_v = is_same<Types...>::value;

} // inline namespace cbu_type_traits
} // namespace cbu
