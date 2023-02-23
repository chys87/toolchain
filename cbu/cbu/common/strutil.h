/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include <algorithm>
#include <concepts>
#include <cstdarg>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <string_view>

#include "cbu/common/concepts.h"

namespace cbu {
namespace strutil_detail {

// Extract traits_type
template <typename T, typename C>
struct CharTraitsType {
  using type = std::char_traits<C>;
};

template <typename T, typename C>
requires requires() { typename T::traits_type; }
struct CharTraitsType<T, C> {
  using type = typename T::traits_type;
};

}  // namespace strutil_detail

// Always return the number of characters actually filled.
// Function name taken from Linux kernel.
std::size_t vscnprintf(char *, std::size_t, const char *, std::va_list) noexcept
  __attribute__((__format__(__printf__, 3, 0)));

std::size_t scnprintf(char *, std::size_t, const char *, ...) noexcept
  __attribute__((__format__(__printf__, 3, 4)));

char *strdup_new(const char *)
  __attribute__((__malloc__, __nonnull__(1)));

std::size_t strcnt(const char *, char) noexcept
  __attribute__((__nonnull__(1), __pure__));

std::size_t memcnt(const char *, char, size_t) noexcept
  __attribute__((__nonnull__(1), __pure__));

inline std::size_t memcnt(std::string_view sv, char c) noexcept {
  return memcnt(sv.data(), c, sv.length());
}

// Same as strcmp, but compares contiguous digits as numbers
int strnumcmp(const char *, const char *) noexcept
  __attribute__((__nonnull__(1, 2), __pure__));

int strnumcmp(std::string_view a, std::string_view b) noexcept
    __attribute__((__pure__));

// Same as std::reverse, but more optimized (at least for x86)
char *reverse(char *, char *) noexcept;

// c_str: Convert anything to a C-style string
inline constexpr const char *c_str(const char *s) noexcept {
  return s;
}

template<typename T>
requires requires(T s) { {s.c_str()} -> std::convertible_to<const char *>; }
inline constexpr const char *c_str(const T &s) noexcept(noexcept(s.c_str())) {
  return s.c_str();
}

template<typename T>
requires requires(T s) { {s->c_str()} -> std::convertible_to<const char *>; }
inline constexpr const char *c_str(const T &s) noexcept(noexcept(s->c_str())) {
  return s->c_str();
}

// Compares two string views, and returns negative, 0, and positive
int compare_string_view_impl(std::string_view a, std::string_view b) noexcept
    __attribute__((__pure__));

inline constexpr int compare_string_view(std::string_view a,
                                         std::string_view b) noexcept {
  if consteval {
    return (a < b) ? -1 : (a == b) ? 0 : 1;
  } else {
    return compare_string_view_impl(a, b);
  }
}

// Returns negative if (a < b), 0 or positive otherwise
int compare_string_view_for_lt_impl(std::string_view a,
                                    std::string_view b) noexcept
    __attribute__((__pure__));

inline constexpr int compare_string_view_for_lt(std::string_view a,
                                                std::string_view b) noexcept {
  if consteval {
    return (a < b) ? -1 : 0;
  } else {
    return compare_string_view_for_lt_impl(a, b);
  }
}

// Convert anything to a string_view, useful in templates.
template <Std_string_char C>
inline constexpr std::basic_string_view<C> any_to_string_view(
    const C* str) noexcept {
  return str;
};

template <typename T>
requires requires(const T& obj) {
  { std::data(obj) } -> Pointer;
  requires Std_string_char<std::remove_cvref_t<decltype(*obj.data())>>;
  { std::size(obj) } -> std::convertible_to<std::size_t>;
}
inline constexpr auto any_to_string_view(const T& obj) noexcept {
  using C = std::remove_cvref_t<decltype(*obj.data())>;
  using Traits = typename strutil_detail::CharTraitsType<T, C>::type;
  return std::basic_string_view<C, Traits>(std::data(obj), std::size(obj));
};

template <typename T>
concept Any_to_string_view_compat = requires(const T& a) {
  {any_to_string_view(a)};
};

// First compare by length, then by string content
// Suitable for use in map and set
template <Any_to_string_view_compat T, Any_to_string_view_compat U>
requires requires(const T& a, const U& b) {
  requires std::is_same_v<decltype(any_to_string_view(a)),
                          decltype(any_to_string_view(b))>;
}
inline constexpr int strcmp_length_first(const T& a, const U& b) noexcept {
  auto sv_a = any_to_string_view(a);
  auto sv_b = any_to_string_view(b);
  if (sv_a.length() < sv_b.length()) {
    return -1;
  } else if (sv_a.length() > sv_b.length()) {
    return 1;
  } else {
    using Traits = typename decltype(sv_a)::traits_type;
    // Explicitly use std::memcmp to make GCC and clang generate better code
    if !consteval {
      if constexpr (std::is_same_v<Traits, std::char_traits<char>> ||
                    std::is_same_v<Traits, std::char_traits<char8_t>>) {
        return std::memcmp(sv_a.data(), sv_b.data(), sv_a.length());
      } else if constexpr (std::is_same_v<Traits, std::char_traits<wchar_t>>) {
        return std::wmemcmp(sv_a.data(), sv_b.data(), sv_a.length());
      }
    }
    return Traits::compare(sv_a.data(), sv_b.data(), sv_a.length());
  }
}

struct StrCmpLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
  constexpr int operator()(const T& a, const U& b) const noexcept {
    return strcmp_length_first(a, b);
  }
};

struct StrLessLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
  constexpr bool operator()(const T& a, const U& b) const noexcept {
    return strcmp_length_first(a, b) < 0;
  }
};

struct StrGreaterLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
  constexpr bool operator()(const T& a, const U& b) const noexcept {
    return strcmp_length_first(a, b) > 0;
  }
};

// Return the length of common prefix.
// If utf8 is true, always cut on UTF-8 character boundaries.
[[gnu::pure]] size_t common_prefix_ex(const void*, const void*, size_t,
                                      bool utf8) noexcept;
[[gnu::pure]] size_t common_suffix_ex(const void*, const void*, size_t,
                                      bool utf8) noexcept;

inline size_t common_prefix(std::string_view sa, std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_prefix_ex(sa.data(), sb.data(), maxl, false);
}

inline size_t common_prefix_utf8(std::string_view sa,
                                 std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_prefix_ex(sa.data(), sb.data(), maxl, true);
}

inline size_t common_prefix(std::u8string_view sa,
                            std::u8string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_prefix_ex(sa.data(), sb.data(), maxl, true);
}

inline size_t common_suffix(std::string_view sa, std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_suffix_ex(sa.end(), sb.end(), maxl, false);
}

inline size_t common_suffix_utf8(std::string_view sa,
                                 std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_suffix_ex(sa.end(), sb.end(), maxl, true);
}

inline size_t common_suffix(std::u8string_view sa,
                            std::u8string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_suffix_ex(sa.end(), sb.end(), maxl, true);
}

// Return the length of prefix composed only of character c
[[gnu::pure]] size_t char_span_length(const void* buffer, size_t len, char c) noexcept;

inline size_t char_span_length(std::string_view s, char c) noexcept {
  return char_span_length(s.data(), s.size(), c);
}

}  // namespace cbu
