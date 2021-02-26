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

#include <compare>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include "concepts.h"

// Provides an immutable string class that either holds a string
// (for flexibility) or a string view (for performance)
// A null-terminator is **not** guaranteed if a string view is used.

namespace cbu {
inline namespace cbu_immutable_string {

template <typename C>
requires Std_string_char<C>
class ImmutableBasicString {
 public:
  using size_type = std::size_t;
  using value_type = const C;
  using reference = const C&;
  using const_reference = const C&;
  using pointer = const C*;
  using iterator = const C*;
  using const_iterator = const C*;

  using string_view = std::basic_string_view<C>;
  using string = std::basic_string<C>;

  enum struct Ref { REF };
  static constexpr Ref REF = Ref::REF;

 public:
  constexpr ImmutableBasicString() noexcept : s_(string_view()) {}

  template <typename...Args>
    requires std::is_constructible_v<string, Args&&...>
  ImmutableBasicString(Args&&...args)
      noexcept(noexcept(string(std::forward<Args>(args)...))) :
    s_(std::in_place_type<string>, std::forward<Args>(args)...) {}

  template <typename...Args>
      requires std::is_constructible_v<string_view, Args&&...>
  constexpr ImmutableBasicString(Ref, Args&&...args)
      noexcept(noexcept(string_view(std::forward<Args>(args)...))) :
    s_(std::in_place_type<string_view>, std::forward<Args>(args)...) {}

  template <typename...Args>
      requires std::is_constructible_v<string_view, Args&&...>
  static constexpr ImmutableBasicString ref(Args&&...args)
      noexcept(noexcept(string_view(std::forward<Args>(args)...))) {
    return ImmutableBasicString(REF, std::forward<Args>(args)...);
  }

  constexpr ImmutableBasicString(const ImmutableBasicString&) = default;
  constexpr ImmutableBasicString(ImmutableBasicString&&) = default;
  constexpr ImmutableBasicString& operator=(const ImmutableBasicString&)
    = default;
  constexpr ImmutableBasicString& operator=(ImmutableBasicString&&) = default;

  constexpr bool holds_reference() const noexcept {
    return std::holds_alternative<string_view>(s_);
  }
  constexpr bool holds_copy() const noexcept {
    return std::holds_alternative<string>(s_);
  }

  constexpr const C* data() const noexcept {
    return dispatch<const C*>([](const auto& v) noexcept { return v.data(); });
  }

  constexpr size_type size() const noexcept {
    return dispatch<size_type>([](const auto& v) noexcept { return v.size(); });
  }
  constexpr size_type length() const noexcept { return size(); }
  [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

  constexpr const C* begin() const noexcept { return data(); }
  constexpr const C* end() const noexcept { return data() + size(); }
  constexpr const C* cbegin() const noexcept { return begin(); }
  constexpr const C* cend() const noexcept { return end(); }

  constexpr string_view view() const noexcept {
    return dispatch<string_view>(
        [](const auto& v) noexcept { return string_view(v); });
  }
  string copy() const {
    return dispatch<string>([](const auto& v) { return string(v); });
  }

  constexpr operator string_view() const noexcept { return view(); }

  friend constexpr  bool operator == (const ImmutableBasicString& a,
                                      const ImmutableBasicString& b) noexcept {
    return a.view() == b.view();
  }
  friend constexpr bool operator != (const ImmutableBasicString& a,
                                     const ImmutableBasicString& b) noexcept {
    return a.view() != b.view();
  }
  friend constexpr bool operator <= (const ImmutableBasicString& a,
                                     const ImmutableBasicString& b) noexcept {
    return a.view() <= b.view();
  }
  friend constexpr bool operator >= (const ImmutableBasicString& a,
                                     const ImmutableBasicString& b) noexcept {
    return a.view() >= b.view();
  }
  friend constexpr bool operator < (const ImmutableBasicString& a,
                                    const ImmutableBasicString& b) noexcept {
    return a.view() < b.view();
  }
  friend constexpr bool operator > (const ImmutableBasicString& a,
                                    const ImmutableBasicString& b) noexcept {
    return a.view() > b.view();
  }
  friend constexpr auto operator <=> (const ImmutableBasicString& a,
                                      const ImmutableBasicString& b) noexcept {
    return a.view() <=> b.view();
  }

 private:
  template <typename Visitor>
  static inline constexpr bool Visitor_noexcept = (
      noexcept(std::declval<Visitor>()(std::declval<string_view>())) &&
      noexcept(std::declval<Visitor>()(std::declval<string>())));

  template <typename R, typename Visitor>
  constexpr R dispatch(Visitor visitor) const
      noexcept(Visitor_noexcept<Visitor>) {
    if (std::holds_alternative<string_view>(s_)) {
      return visitor(std::get<string_view>(s_));
    } else if (std::holds_alternative<string>(s_)) {
      return visitor(std::get<string>(s_));
    } else {
      return R();
    }
  }

 private:
  std::variant<string_view, string> s_;
};

using ImmutableString = ImmutableBasicString<char>;
using ImmutableWString = ImmutableBasicString<wchar_t>;

} // namespace cbu_immutable_string
} // namespace cbu
