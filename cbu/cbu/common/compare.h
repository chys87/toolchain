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

// A wrapper around another comparator, which may either be a predicate
// (returning a boolean value), or a three-way comparator (returning
// int, std::weak_ordering, or std::strong_ordering)

#include <compare>
#include <concepts>
#include <type_traits>

namespace cbu::inline cbu_compare {

template <typename Comp, typename U, typename V>
concept Three_way_comparator = requires(Comp& comp, const U& u, const V& v) {
  { comp(u, v) };
  requires (std::is_same_v<std::remove_cvref_t<decltype(comp(u, v))>, int> ||
            std::is_same_v<std::remove_cvref_t<decltype(comp(u, v))>,
                           std::weak_ordering> ||
            std::is_same_v<std::remove_cvref_t<decltype(comp(u, v))>,
                           std::strong_ordering>);
};

template <typename Comp, typename U, typename V>
requires (Three_way_comparator<const Comp, U, V> &&
          !std::predicate<const Comp, U, V>)
constexpr auto compare_three_way_with(const Comp& comp, const U& u, const V& v)
    noexcept(noexcept(comp(u, v))) {
  return comp(u, v);
}

template <typename Comp, typename U, typename V>
requires (!Three_way_comparator<const Comp, U, V> &&
          std::predicate<const Comp, U, V>)
constexpr std::weak_ordering compare_three_way_with(
    const Comp& comp, U&& u, V&& v)
    noexcept(noexcept(comp(u, v)) && noexcept(comp(v, u))) {
  if (comp(u, v))
    return std::weak_ordering::less;
  else if (comp(v, u))
    return std::weak_ordering::greater;
  else
    return std::weak_ordering::equivalent;
}

template <typename Comp, typename U, typename V>
requires (Three_way_comparator<const Comp, U, V> &&
          !std::predicate<const Comp, U, V>)
constexpr bool compare_lt_with(const Comp& comp, const U& u, const V& v)
    noexcept(noexcept(comp(u, v))) {
  return comp(u, v) < 0;
}

template <typename Comp, typename U, typename V>
requires (!Three_way_comparator<const Comp, U, V> &&
          std::predicate<const Comp, U, V>)
constexpr bool compare_lt_with(const Comp& comp, U&& u, V&& v)
    noexcept(noexcept(comp(u, v))) {
  return comp(u, v);
}

} // namespace cbu::inline cbu_compare
