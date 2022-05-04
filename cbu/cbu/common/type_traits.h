/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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

#include <array>
#include <concepts>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace cbu {

// Check whether a type is one of many types
template <typename T, typename... C>
struct is_one_of : std::disjunction<std::is_same<T, C>...> {};

template <typename T, typename... C>
inline constexpr bool is_one_of_v = is_one_of<T, C...>::value;

// Extended is_same
template <typename...> struct is_same;
template <> struct is_same<> : std::true_type {};
template <typename U, typename... V>
struct is_same<U, V...> : std::conjunction<std::is_same<U, V>...> {};

template <typename... Types>
inline constexpr bool is_same_v = is_same<Types...>::value;

// bitwise_movable<T> bitwise_movable_v<T>:
//  If true, it means type T is "bitwise movable", i.e.
//  an instance of T can be safely bitwise moved to another address, and it
//  is still a valid object without change in semantics.
//  Clang calls this "trivially relocatable".
//
// pass_type_t<T>: Equivalent to T or const T& depending on its "simplicity"
// pass_type_t<T, T&&>: Equivalent to T or T&& depending on its "simplicity"
//
// User can define a member function to alter the above mentioned traits, e.g.:
//  static inline constexpr bool bitwise_movable(T*) { return true; }
//  static inline constexpr bool pass_by_ref(T*) { return true; }
//  static inline constexpr bool pass_by_value(T*) { return true; }
// The parameter is used to prevent the traits from being inherited
//
// The "bitwise movable" traits can also be "chained", e.g.
//  template <typename First, typename Second>
//  strcut MyPair{
//    First first;
//    Second second;
//    using bitwise_movable_chain = MyPair(First, Second);
//  };
// This means MyPair<First, Second> if bitwise movable if and only if both
// First and Second are bitwise movable.


namespace type_traits_detail {

#define CBU_DEFINE_EXTRACT_TRAITS(name, vtype)                               \
  template <typename Class, vtype fallback = vtype()>                        \
  struct extract_traits_##name : std::integral_constant<vtype, fallback> {}; \
  template <typename Class, vtype fallback>                                  \
  requires requires() {                                                      \
    static_cast<vtype (*)(Class*)>(&Class::name);                            \
    { Class::name(nullptr) } -> std::convertible_to<vtype>;                  \
  }                                                                          \
  struct extract_traits_##name<Class, fallback>                              \
      : std::integral_constant<vtype, Class::name(nullptr)> {}

CBU_DEFINE_EXTRACT_TRAITS(bitwise_movable, bool);
CBU_DEFINE_EXTRACT_TRAITS(pass_by_ref, bool);
CBU_DEFINE_EXTRACT_TRAITS(pass_by_value, bool);

template <typename, typename>
struct is_chain_type : std::false_type {};
template <typename R, typename... Args>
struct is_chain_type<R, R(Args...)> : std::true_type {};

template <template <typename> class Trait, typename ChainType>
struct and_chain;

template <template <typename> class Trait, typename R, typename... Args>
struct and_chain<Trait, R(Args...)> : std::conjunction<Trait<Args>...>::type {};

}  // namespace type_traits_detail

template <typename T>
struct is_trivially_copyable_and_destructible
    : std::conjunction<std::is_trivially_copyable<T>,
                       std::is_trivially_destructible<T>>::type {};

template <typename T>
struct bitwise_movable;

template <typename T>
struct bitwise_movable
    : std::disjunction<
          is_trivially_copyable_and_destructible<T>,
#if __has_builtin(__is_trivially_relocatable)  // Clang 15
          std::bool_constant<__is_trivially_relocatable(T)>,
#endif
          type_traits_detail::extract_traits_bitwise_movable<T>>::type {};

template <typename T>
requires type_traits_detail::is_chain_type<
    T, typename T::bitwise_movable_chain>::value struct bitwise_movable<T>
    : type_traits_detail::and_chain<bitwise_movable,
                                    typename T::bitwise_movable_chain>::type {
};

template <typename A, typename B>
struct bitwise_movable<std::pair<A, B>>
    : std::conjunction<bitwise_movable<A>, bitwise_movable<B>>::type {};

template <typename... T>
struct bitwise_movable<std::tuple<T...>>
    : std::conjunction<bitwise_movable<T>...>::type {};

template <typename... T>
struct bitwise_movable<std::variant<T...>>
    : std::conjunction<bitwise_movable<T>...>::type {};

template <typename T>
struct bitwise_movable<std::optional<T>> : bitwise_movable<T>::type {};

// unique_ptr holds a deleter, so it must be bitwise movable as well
template <typename T, typename Deleter>
struct bitwise_movable<std::unique_ptr<T, Deleter>>
    : bitwise_movable<Deleter>::type {};

template <typename T>
struct bitwise_movable<std::shared_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool bitwise_movable_v = bitwise_movable<T>::value;

namespace type_traits_test_1 {

struct A {};
struct B : A {
  static constexpr bool bitwise_movable(B*) { return true; }
};
struct C : B {};

static_assert(!type_traits_detail::extract_traits_bitwise_movable<A>::value,
              "");
static_assert(type_traits_detail::extract_traits_bitwise_movable<B>::value, "");
static_assert(!type_traits_detail::extract_traits_bitwise_movable<C>::value,
              "");

struct F {
  F(const F&);
};
struct T1 {};
struct T2 : F {
  static constexpr bool bitwise_movable(T2*) { return true; }
};

template <typename A, typename B>
struct pair1 {
  using bitwise_movable_chain = pair1(A, B);
};
template <typename A, typename B>
struct pair2 : pair1<A, B> {
  pair2(const pair2&);
};

static_assert(bitwise_movable_v<T1>, "");
static_assert(bitwise_movable_v<T2>, "");
static_assert(!bitwise_movable_v<F>, "");
static_assert(bitwise_movable_v<pair1<T1, T2>>, "");
static_assert(bitwise_movable_v<pair1<T1, T1>>, "");
static_assert(!bitwise_movable_v<pair1<T1, F>>, "");
static_assert(!bitwise_movable_v<pair1<T2, F>>, "");
static_assert(!bitwise_movable_v<pair2<T1, T2>>, "");
static_assert(bitwise_movable_v<std::pair<int, char>>);
static_assert(!bitwise_movable_v<std::pair<int, F>>);
static_assert(bitwise_movable_v<std::tuple<int, long, T1, T2>>);
static_assert(!bitwise_movable_v<std::tuple<int, long, T1, T2, F>>);
static_assert(bitwise_movable_v<std::array<T1, 2>>);
static_assert(bitwise_movable_v<std::unique_ptr<int>>);
static_assert(bitwise_movable_v<std::unique_ptr<F>>);

}  // namespace type_traits_test_1

template <typename T>
struct default_pass_by_value
    : std::bool_constant<is_trivially_copyable_and_destructible<T>::value &&
                         sizeof(T) <= 2 * sizeof(void*)> {};

template <typename T>
struct default_pass_by_value<T[]> : std::false_type {};
template <typename T, std::size_t N>
struct default_pass_by_value<T[N]> : std::false_type {};

template <typename T>
struct pass_by_value
    : std::disjunction<
          type_traits_detail::extract_traits_pass_by_value<T>,
          std::conjunction<
              std::negation<type_traits_detail::extract_traits_pass_by_ref<T>>,
              default_pass_by_value<T>>>::type {};

template <typename T, typename F = const std::remove_cvref_t<T>&>
struct pass_type
    : std::conditional<pass_by_value<std::remove_cvref_t<T>>::value,
                       std::remove_cvref_t<T>, F> {};

// For T[N], the non-specialized one (const T (&)[N]) is good enough
template <typename T, typename F>
struct pass_type<T[], F> {
  using type = const T*;
};

template <typename T, typename F = const std::remove_cvref_t<T>&>
using pass_type_t = typename pass_type<T, F>::type;

}  // namespace cbu
