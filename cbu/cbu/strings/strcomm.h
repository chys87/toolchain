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

#include <cstring>
#include <string_view>

#include "cbu/common/concepts.h"
#include "cbu/compat/compilers.h"

namespace cbu {
namespace strcomm_detail {

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

}  // namespace strcomm_detail

// c_str: Convert anything to a C-style string
inline constexpr const char* c_str(const char* s) noexcept { return s; }

template <typename T>
  requires requires(const T& s) {
    { s.c_str() } -> std::convertible_to<const char*>;
  }
inline constexpr const char* c_str(const T& s) noexcept(noexcept(s.c_str())) {
  return s.c_str();
}

template <typename T>
  requires requires(T s) {
    { s->c_str() } -> std::convertible_to<const char*>;
  }
inline constexpr const char* c_str(const T& s) noexcept(noexcept(s->c_str())) {
  return s->c_str();
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
  using Traits = typename strcomm_detail::CharTraitsType<T, C>::type;
  return std::basic_string_view<C, Traits>(std::data(obj), std::size(obj));
};

template <typename T>
concept Any_to_string_view_compat = requires(const T& a) {
  { any_to_string_view(a) };
};

template <typename T, typename U>
concept Any_to_string_view_compat_2 =
    Any_to_string_view_compat<T> && Any_to_string_view_compat<U> &&
    requires(const T& a, const U& b) {
      requires std::is_same_v<decltype(any_to_string_view(a)),
                              decltype(any_to_string_view(b))>;
    };

// Equivalent to, but faster than libstdc++ version of, a.compare(b)
template <typename T, typename U>
  requires Any_to_string_view_compat_2<T, U>
constexpr int compare_string_view(const T& va, const U& vb) noexcept {
  auto a = any_to_string_view(va);
  auto b = any_to_string_view(vb);
  if consteval {
    return (a < b) ? -1 : (a == b) ? 0 : 1;
  } else {
    size_t l = a.size() < b.size() ? a.size() : b.size();
    int rc;
    using Traits = typename decltype(a)::traits_type;
    if constexpr (std::is_same_v<Traits, std::char_traits<char>> ||
                  std::is_same_v<Traits, std::char_traits<char8_t>>) {
      rc = std::memcmp(a.data(), b.data(), l);
    } else if constexpr (std::is_same_v<Traits, std::char_traits<wchar_t>>) {
      rc = std::wmemcmp(a.data(), b.data(), l);
    } else {
      rc = Traits::compare(a.data(), b.data(), l);
    }
    if (rc == 0)
      rc = (a.size() < b.size()) ? -1 : (a.size() == b.size()) ? 0 : 1;
    return rc;
  }
}

// Returns (a < b), implemented in an optimized way (compared to libstdc++)
template <typename T, typename U>
  requires Any_to_string_view_compat_2<T, U>
constexpr int string_view_lt(const T& va, const U& vb) noexcept {
  auto a = any_to_string_view(va);
  auto b = any_to_string_view(vb);
  if consteval {
    return (a < b);
  } else {
    size_t l = a.size() < b.size() ? a.size() : b.size();
    using Traits = typename decltype(a)::traits_type;
    int rc;
    if consteval {
      rc = Traits::compare(a.data(), b.data(), l);
    } else {
      if constexpr (std::is_same_v<Traits, std::char_traits<char>> ||
                    std::is_same_v<Traits, std::char_traits<char8_t>>) {
        rc = std::memcmp(a.data(), b.data(), l);
      } else if constexpr (std::is_same_v<Traits, std::char_traits<wchar_t>>) {
        rc = std::wmemcmp(a.data(), b.data(), l);
      } else {
        rc = Traits::compare(a.data(), b.data(), l);
      }
    }
    return (a.size() < b.size()) ? (rc <= 0) : (rc < 0);
  }
}

// First compare by length, then by string content
// Suitable for use in map and set
template <typename T, typename U>
  requires Any_to_string_view_compat_2<T, U>
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

struct StrLess {
  using is_transparent = void;

  template <typename T, typename U>
    requires Any_to_string_view_compat_2<T, U>
  static constexpr int operator()(const T& a, const U& b) noexcept {
    return string_view_lt(a, b);
  }
};

struct StrGreater {
  using is_transparent = void;

  template <typename T, typename U>
    requires Any_to_string_view_compat_2<T, U>
  static constexpr int operator()(const T& a, const U& b) noexcept {
    return string_view_lt(b, a);
  }
};

struct StrCmpLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
    requires Any_to_string_view_compat_2<T, U>
  static constexpr int operator()(const T& a, const U& b) noexcept {
    return strcmp_length_first(a, b);
  }
};

struct StrLessLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
    requires Any_to_string_view_compat_2<T, U>
  static constexpr bool operator()(const T& a, const U& b) noexcept {
    return strcmp_length_first(a, b) < 0;
  }
};

struct StrGreaterLengthFirst {
  using is_transparent = void;

  template <typename T, typename U>
    requires Any_to_string_view_compat_2<T, U>
  static constexpr bool operator()(const T& a, const U& b) noexcept {
    return strcmp_length_first(a, b) > 0;
  }
};

}  // namespace cbu
