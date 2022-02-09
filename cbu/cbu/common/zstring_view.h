/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2021, chys <admin@CHYS.INFO>
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

#include <string_view>

#include "cbu/common/concepts.h"
#include "cbu/common/strutil.h"

namespace cbu {

// basic_zstring_view is like string_view, but the user can safely
// assume a null terminator is present
template <typename C, typename T = std::char_traits<C>>
class basic_zstring_view : public std::basic_string_view<C, T> {
 public:
  struct UnsafeRef {};
  static constexpr UnsafeRef UNSAFE_REF {};

  using Base = std::basic_string_view<C, T>;

 public:
  constexpr basic_zstring_view() noexcept = default;
  constexpr basic_zstring_view(const C* s) noexcept : Base(s) {}

  // Caller's responsibility to guarantee the presence of a null terminator!!
  constexpr basic_zstring_view(const C* s, std::size_t l) noexcept
      : Base(s, l) {}
  constexpr basic_zstring_view(const C* s, const C* e) noexcept
      : Base(s, e - s) {}

  constexpr basic_zstring_view(const basic_zstring_view&) noexcept = default;
  constexpr basic_zstring_view& operator=(const basic_zstring_view&) noexcept =
      default;

  template <Zstring_view_compat<C> S>
  requires(!std::is_same<basic_zstring_view, S>::value &&
           !std::is_base_of<basic_zstring_view, S>::
               value) constexpr basic_zstring_view(const S& s) noexcept
      : Base(s.c_str(), s.length()) {}

  template <String_view_compat<C> S>
  constexpr basic_zstring_view(UnsafeRef, const S& s) noexcept : Base(s) {}

  using Base::data;
  using Base::size;
  using Base::length;
  constexpr const C* c_str() const noexcept { return data(); }

  using Base::compare;
  constexpr int compare(const C* o) const noexcept {
    if (std::is_constant_evaluated()) {
      return compare(Base(o));
    } else if constexpr (std::is_same_v<C, char> &&
                         std::is_same_v<T, std::char_traits<char>>) {
      return __builtin_strcmp(c_str(), o);
    } else if constexpr (std::is_same_v<C, wchar_t> &&
                         std::is_same_v<T, std::char_traits<wchar_t>>) {
      return __builtin_wcscmp(c_str(), o);
    } else {
      return Base(*this).compare(o);
    }
  }

  // Return a reference to a static empty string_view instance.
  // Sometimes it's convenience to point to it rather than nullptr.
  static const basic_zstring_view& empty_instance() noexcept;

  constexpr void remove_suffix(std::size_t) = delete;

  constexpr Base substr(std::size_t pos, std::size_t count) const noexcept {
    return Base::substr(pos, count);
  }
  constexpr basic_zstring_view substr(std::size_t pos) const noexcept {
    return basic_zstring_view(data() + pos, length() - pos);
  }

  // Returns a string_view including the null-terminiator
  constexpr Base as_svz() const noexcept { return {data(), size() + 1}; }
};

template <typename C, typename T>
inline const basic_zstring_view<C, T>&
basic_zstring_view<C, T>::empty_instance() noexcept {
  if constexpr (std::is_same_v<C, char>) {
    static constinit const basic_zstring_view<C, T> empty{"", std::size_t(0)};
    return empty;
  } else if constexpr (std::is_same_v<C, wchar_t>) {
    static constinit const basic_zstring_view<C, T> empty{L"", std::size_t(0)};
    return empty;
  } else if constexpr (std::is_same_v<C, char8_t>) {
    static constinit const basic_zstring_view<C, T> empty{u8"", std::size_t(0)};
    return empty;
  } else if constexpr (std::is_same_v<C, char16_t>) {
    static constinit const basic_zstring_view<C, T> empty{u"", std::size_t(0)};
    return empty;
  } else if constexpr (std::is_same_v<C, char32_t>) {
    static constinit const basic_zstring_view<C, T> empty{U"", std::size_t(0)};
    return empty;
  } else {
    static constinit const C empty_str[1] = {};
    static constinit const basic_zstring_view<C, T> empty{empty_str,
                                                          std::size_t(0)};
    return empty;
  }
}

template <typename C, typename T>
inline constexpr bool operator==(const basic_zstring_view<C, T>& a,
                                 const C* b) noexcept {
  return a.compare(b) == 0;
}

template <typename C, typename T>
inline constexpr int operator<=>(const basic_zstring_view<C, T>& a,
                                 const C* b) noexcept {
  return a.compare(b);
}

template <typename C, typename T>
inline constexpr int compare(const basic_zstring_view<C, T>& a,
                             const basic_zstring_view<C, T>& b) noexcept {
  return a.compare(b);
}

template <typename C, typename T>
inline constexpr int compare(const basic_zstring_view<C, T>& a,
                             const C* b) noexcept {
  return a.compare(b);
}

template <typename C, typename T>
inline constexpr int compare(const C* a,
                             const basic_zstring_view<C, T>& b) noexcept {
  return -compare(b, a);
}

template <typename C, typename T>
inline constexpr bool operator<(const basic_zstring_view<C, T>& a,
                                const basic_zstring_view<C, T>& b) noexcept {
  if constexpr (std::is_same_v<C, char> &&
                std::is_same_v<T, std::char_traits<char>>) {
    return compare_string_view_for_lt(a, b) < 0;
  }
  return std::basic_string_view<C, T>(a) < std::basic_string_view<C, T>(b);
}

template <typename C, typename T>
inline constexpr bool operator>(const basic_zstring_view<C, T>& a,
                                const basic_zstring_view<C, T>& b) noexcept {
  return (b < a);
}

template <typename C, typename T>
inline constexpr bool operator<=(const basic_zstring_view<C, T>& a,
                                 const basic_zstring_view<C, T>& b) noexcept {
  return !(a > b);
}

template <typename C, typename T>
inline constexpr bool operator>=(const basic_zstring_view<C, T>& a,
                                 const basic_zstring_view<C, T>& b) noexcept {
  return !(a < b);
}

#define CBU_ZSTRING_VIEW_LITERALS(CharType)                         \
  inline constexpr basic_zstring_view<CharType> operator""_sv(      \
      const CharType* s, std::size_t l) noexcept {                  \
    return {s, l};                                                  \
  }                                                                 \
  /* Including the null terminiator*/                               \
  inline constexpr std::basic_string_view<CharType> operator""_svz( \
      const CharType* s, std::size_t l) noexcept {                  \
    return {s, l + 1};                                              \
  }

CBU_ZSTRING_VIEW_LITERALS(char)
CBU_ZSTRING_VIEW_LITERALS(wchar_t)
CBU_ZSTRING_VIEW_LITERALS(char8_t)
CBU_ZSTRING_VIEW_LITERALS(char16_t)
CBU_ZSTRING_VIEW_LITERALS(char32_t)

#undef CBU_ZSTRING_VIEW_LITERALS

using zstring_view = basic_zstring_view<char>;
using wzstring_view = basic_zstring_view<wchar_t>;
using u8zstring_view = basic_zstring_view<char8_t>;
using u16zstring_view = basic_zstring_view<char16_t>;
using u32zstring_view = basic_zstring_view<char32_t>;

// Test constexpr comparisons
static_assert("abc"_sv > "123"_sv);
// As of GCC 10, there's a bug that fails the following assertion.
// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99181)
// TODO: Uncomment it when it's fixed.
// static_assert("\xff"_sv > "aaa"_sv);

} // namespace cbu
