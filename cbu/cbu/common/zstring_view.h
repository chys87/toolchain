/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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

// zstring_view is like string_view, but the user can safely
// assume a null terminator is present
class zstring_view : public std::string_view {
 public:
  struct UnsafeRef {};
  static constexpr UnsafeRef UNSAFE_REF {};

 public:
  constexpr zstring_view() noexcept = default;
  constexpr zstring_view(const char *s) noexcept : std::string_view(s) {}

  // Caller's responsibility to guarantee the presence of a null terminator!!
  constexpr zstring_view(const char *s, std::size_t l) noexcept :
    std::string_view(s, l) {}
  constexpr zstring_view(const char *s, const char *e) noexcept :
    std::string_view(s, e - s) {}

  constexpr zstring_view(const zstring_view &) noexcept = default;
  constexpr zstring_view &operator = (const zstring_view &) noexcept = default;

  template <Zstring_view_compat S>
    requires (!std::is_same<zstring_view, S>::value &&
              !std::is_base_of<zstring_view, S>::value)
  constexpr zstring_view(const S &s) noexcept :
    std::string_view(s.c_str(), s.length()) {}

  template <String_view_compat S>
  constexpr zstring_view(UnsafeRef, const S &s) noexcept :
    std::string_view(s) {}

  constexpr const char *c_str() const noexcept { return data(); }

  using std::string_view::compare;
  constexpr int compare(const char *o) const noexcept {
    if (std::is_constant_evaluated()) {
      return compare(std::string_view(o));
    } else {
      return __builtin_strcmp(c_str(), o);
    }
  }

  // Return a reference to a static empty string_view instance.
  // Sometimes it's convenience to point to it rather than nullptr.
  static const zstring_view &empty_instance() noexcept;

  constexpr void remove_suffix(std::size_t) = delete;

  constexpr std::string_view substr(std::size_t pos, std::size_t count) const {
    return std::string_view::substr(pos, count);
  }
  constexpr zstring_view substr(std::size_t pos) const {
    return zstring_view(data() + pos, length() - pos);
  }
};

inline const zstring_view empty_zstring_view_instance("", std::size_t(0));

inline const zstring_view &zstring_view::empty_instance() noexcept {
  return empty_zstring_view_instance;
}

inline constexpr bool operator == (const zstring_view &a,
                                   const char *b) noexcept {
  return a.compare(b) == 0;
}

inline constexpr int operator <=> (const zstring_view &a,
                                   const char *b) noexcept {
  return a.compare(b);
}

inline constexpr int compare(const zstring_view &a,
                             const zstring_view &b) noexcept {
  return a.compare(b);
}

inline constexpr int compare(const zstring_view &a, const char *b) noexcept {
  return a.compare(b);
}

inline constexpr int compare(const char *a, const zstring_view &b) noexcept {
  return -compare(b, a);
}

inline constexpr bool operator<(const zstring_view &a,
                                const zstring_view &b) noexcept {
  if (std::is_constant_evaluated()) {
    return std::string_view(a) < std::string_view(b);
  } else {
    return compare_string_view_for_lt(a, b) < 0;
  }
}
inline constexpr bool operator>(const zstring_view &a,
                                const zstring_view &b) noexcept {
  return (b < a);
}

inline constexpr bool operator<=(const zstring_view &a,
                                 const zstring_view &b) noexcept {
  return !(a > b);
}

inline constexpr bool operator>=(const zstring_view &a,
                                 const zstring_view &b) noexcept {
  return !(a < b);
}

inline constexpr zstring_view operator""_sv(const char *s, size_t l) noexcept {
  return {s, l};
}

// Test constexpr comparisons
static_assert("abc"_sv > "123"_sv);
// As of GCC 10, there's a bug that fails the following assertion.
// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99181)
// TODO: Uncomment it when it's fixed.
// static_assert("\xff"_sv > "aaa"_sv);

} // namespace cbu
