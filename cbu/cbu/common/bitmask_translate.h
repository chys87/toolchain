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

#include <array>
#include <limits>
#include <tuple>

#include "cbu/common/bit.h"
#include "cbu/common/concepts.h"

namespace cbu {
namespace bitmask_translate_detail {

template <Raw_unsigned_integral T>
constexpr int rshift_bits(T f, T t) noexcept {
  if (f == t || f == 0 || t == 0) {
    return 0;
  } else if (f > t) {
    return std::countr_zero(f / t);
  } else {
    return -std::countr_zero(t / f);
  }
}

constexpr auto shift(Raw_unsigned_integral auto v, int rb) noexcept {
  return rb >= 0 ? v >> rb : v << -rb;
}

template <typename T>
struct normalize {
  using type = std::conditional_t<sizeof(T) <= sizeof(unsigned), unsigned,
                                  std::make_unsigned_t<T>>;
};

template <typename T>
  requires std::is_enum_v<T>
struct normalize<T> {
  using type = std::make_unsigned_t<std::underlying_type_t<T>>;
};

template <typename T>
using normalize_t = typename normalize<T>::type;

template <Raw_unsigned_integral... Types>
struct common_type {
  static constexpr auto S = std::max({sizeof(unsigned), sizeof(Types)...});
  using type =
      std::conditional_t<S <= sizeof(unsigned), unsigned,
                         std::conditional_t<S <= sizeof(unsigned long),
                                            unsigned long, unsigned long long>>;
};

template <Raw_unsigned_integral... T>
using common_type_t = typename common_type<T...>::type;

template <typename T>
concept Type = std::is_integral_v<T> || std::is_enum_v<T>;

template <Raw_unsigned_integral T>
struct Pair {
  T f;
  T t;

  constexpr Pair(Type auto f, Type auto t) noexcept : f(f), t(t) {}
};

template <Type F, Type T>
Pair(F, T) -> Pair<common_type_t<normalize_t<F>, normalize_t<T>>>;

template <Raw_unsigned_integral T>
struct Group {
  T f;
  int rb;

  constexpr Group(T f, int rb) noexcept : f(f), rb(rb) {}

  template <Raw_unsigned_integral U>
    requires(sizeof(T) >= sizeof(U))
  constexpr Group(Pair<U> p) noexcept : f(p.f), rb(rshift_bits(p.f, p.t)) {}
};

template <Raw_unsigned_integral T, Group<T>... groups>
struct GroupSet {};

template <Raw_unsigned_integral T, Group<T>... lgroups, Group<T>... rgroups>
constexpr auto operator+(GroupSet<T, lgroups...> l, GroupSet<T, rgroups...> r) {
  return GroupSet<T, lgroups..., rgroups...>{};
}

template <typename GroupSetType, int min_rb, int max_rb>
struct FilterGroupSetByRb;

template <Raw_unsigned_integral T, int min_rb, int max_rb>
struct FilterGroupSetByRb<GroupSet<T>, min_rb, max_rb> {
  using type = GroupSet<T>;
};

template <Raw_unsigned_integral T, Group<T> g0, Group<T>... g, int min_rb,
          int max_rb>
  requires(g0.rb >= min_rb && g0.rb <= max_rb)
struct FilterGroupSetByRb<GroupSet<T, g0, g...>, min_rb, max_rb> {
  using type = decltype(GroupSet<T, g0>() +
                        typename FilterGroupSetByRb<GroupSet<T, g...>, min_rb,
                                                    max_rb>::type());
};

template <Raw_unsigned_integral T, Group<T> g0, Group<T>... g, int min_rb,
          int max_rb>
  requires(g0.rb < min_rb || g0.rb > max_rb)
struct FilterGroupSetByRb<GroupSet<T, g0, g...>, min_rb, max_rb> {
  using type =
      typename FilterGroupSetByRb<GroupSet<T, g...>, min_rb, max_rb>::type;
};

template <typename GroupSetType, int min_rb, int max_rb>
using FilterGroupSetByRb_t =
    typename FilterGroupSetByRb<GroupSetType, min_rb, max_rb>::type;

template <Raw_unsigned_integral T, Group<T>... lgroups, Group<T> rgroup>
constexpr auto operator|(GroupSet<T, lgroups...> l, GroupSet<T, rgroup> r) {
  if constexpr ((false || ... || (lgroups.rb == rgroup.rb))) {
    auto merge_if_rb_equal =
        [](const Group<T>& lgroup) constexpr noexcept {
          if (lgroup.rb == rgroup.rb) {
            return Group<T>{lgroup.f | rgroup.f, lgroup.rb};
          } else {
            return lgroup;
          }
        };
    return GroupSet<T, merge_if_rb_equal(lgroups)...>{};
  } else {
    // Insert sort.
    constexpr auto m = std::numeric_limits<int>::min();
    constexpr auto M = std::numeric_limits<int>::max();
    return FilterGroupSetByRb_t<GroupSet<T, lgroups...>, m, rgroup.rb - 1>() +
           r +
           FilterGroupSetByRb_t<GroupSet<T, lgroups...>, rgroup.rb + 1, M>();
  }
}

}  // namespace bitmask_translate_detail

template <bitmask_translate_detail::Pair... pairs>
  requires((true && ... && is_pow2(pairs.f)) && ... && is_pow2(pairs.t))
constexpr auto bitmask_translate(
    std::common_type_t<decltype(pairs.f)...> input) noexcept {
  if constexpr (sizeof...(pairs) == 0) {
    return 0u;
  } else {
    using namespace bitmask_translate_detail;
    using T = common_type_t<decltype(pairs.f)..., decltype(pairs.t)...>;

    constexpr auto groups = (... | GroupSet<T, Group<T>{pairs}>{});
    return [input]<auto... g>(const GroupSet<T, g...>&) constexpr noexcept {
      return (T() | ... | shift(input & g.f, g.rb));
    }(groups);
  }
}

static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(1) == 1);
static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(7) == 11);
static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(0) == 0);
static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(4) == 8);
static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(5) == 9);
static_assert(bitmask_translate<{1, 1}, {2, 2}, {4, 8}>(37) == 9);

}  // namespace cbu
