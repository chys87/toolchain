/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2025, chys <admin@CHYS.INFO>
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

#ifdef __AVX2__
#include <x86intrin.h>
#endif

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cbu/common/tags.h"  // IWYU pragma: export
#include "cbu/strings/zstring_view.h"

namespace cbu {

template <std::size_t MaxLen>
using strlen_t = std::conditional_t<
    (MaxLen < 65536),
    std::conditional_t<(MaxLen < 256), std::uint8_t, std::uint16_t>, size_t>;

template <typename Builder, std::size_t MaxLen = 0x7fff'ffff>
concept BuilderForShortString = requires(const Builder& bldr) {
  requires(sizeof(char[Builder::static_max_size()]) <= MaxLen);
  { bldr.write(std::declval<char*>()) } -> std::same_as<char*>;
};

template <std::size_t MaxLen, bool Z>
class basic_short_string {
 public:
  using len_t = strlen_t<MaxLen>;

  explicit basic_short_string(UninitializedTag) noexcept {}

  constexpr basic_short_string() noexcept : s_{}, l_{0} {}

  template <std::size_t LenP1>
  consteval basic_short_string(const char (&src)[LenP1]) noexcept
      : basic_short_string(std::string_view(src, LenP1 - 1)) {
    // Use a static assertion rather than a concept, to prevent
    // longer strings from matching the following std::string_view overload.
    static_assert(LenP1 <= MaxLen + 1);
  }

  constexpr basic_short_string(std::string_view src) noexcept {
    if consteval {
      std::fill_n(s_, sizeof(s_), 0);
    }
    assign(src);
  }

  template <BuilderForShortString<MaxLen> Builder>
  constexpr explicit basic_short_string(const Builder& builder) noexcept {
    if consteval {
      std::fill_n(s_, sizeof(s_), 0);
    }
    char* w = builder.write(s_);
    if constexpr (Z) {
      *w = '\0';
    }
    l_ = w - s_;
  }

  constexpr char (&buffer() noexcept)[MaxLen + Z] { return s_; }
  constexpr void set_length(len_t l) { l_ = l; }

  constexpr const char* c_str() const noexcept
    requires Z
  {
    return s_;
  }
  constexpr const char* data() const noexcept { return s_; }
  constexpr char operator[](std::size_t n) const noexcept { return s_[n]; }
  constexpr len_t length() const noexcept { return l_; }
  constexpr len_t size() const noexcept { return l_; }
  constexpr bool empty() const noexcept { return l_ == 0; }
  constexpr const char* begin() const noexcept { return s_; }
  constexpr const char* end() const noexcept { return s_ + l_; }
  constexpr const char* cbegin() const noexcept { return s_; }
  constexpr const char* cend() const noexcept { return s_ + l_; }
  constexpr operator std::string_view() const noexcept { return {s_, l_}; }
  constexpr operator zstring_view() const noexcept
    requires Z
  {
    return {s_, l_};
  }

  constexpr std::conditional_t<Z, zstring_view, std::string_view> string_view()
      const noexcept {
    return {s_, l_};
  }
  std::string string() const { return std::string(s_, l_); }

  static constexpr len_t capacity() noexcept { return MaxLen; }

  constexpr void assign(std::string_view rhs) noexcept {
    if consteval {
      [[maybe_unused]] auto copy_end =
          std::copy_n(rhs.data(), rhs.length(), s_);
      if constexpr (Z) {
        *copy_end = '\0';
      }
    } else {
      // memcpy is fine, but copy_n uses memmove
      std::memcpy(s_, rhs.data(), rhs.length());
      if constexpr (Z) {
        s_[rhs.length()] = '\0';
      }
    }
    l_ = rhs.length();
  }

  struct StrBuilder {
    const basic_short_string& s;

    static constexpr len_t static_max_size() noexcept { return MaxLen; }
    static constexpr len_t max_size() noexcept { return MaxLen; }
    constexpr len_t min_size() const noexcept { return s.l_; }
    constexpr char* write(char* w) const noexcept {
      if consteval {
        std::copy_n(s.s_, s.l_, w);
      } else {
        std::memcpy(w, s.s_, MaxLen);
      }
      return w + s.l_;
    }
  };
  constexpr StrBuilder str_builder() const noexcept { return StrBuilder{*this}; }

 private:
  char s_[MaxLen + Z];
  len_t l_;
};

template <std::size_t MaxLen>
using short_string = basic_short_string<MaxLen, true>;

template <std::size_t MaxLen>
using short_nzstring = basic_short_string<MaxLen, false>;

template <std::size_t N>
  requires(N > 0)
basic_short_string(const char (&)[N]) -> basic_short_string<N - 1, true>;

template <std::size_t N>
  requires(N > 0)
basic_short_string(const char (&)[N]) -> basic_short_string<N - 1, false>;

template <BuilderForShortString<> Builder>
basic_short_string(const Builder&)
    -> basic_short_string<Builder::static_max_size(), true>;

template <BuilderForShortString<> Builder>
basic_short_string(const Builder&)
    -> basic_short_string<Builder::static_max_size(), false>;

template <std::size_t M, bool Z1, std::size_t N, bool Z2>
constexpr bool operator==(const basic_short_string<M, Z1>& a,
                          const basic_short_string<N, Z2>& b) noexcept {
  return a.string_view() == b.string_view();
}

template <std::size_t M, bool Z1, std::size_t N, bool Z2>
constexpr std::strong_ordering operator<=>(
    const basic_short_string<M, Z1>& a,
    const basic_short_string<N, Z2>& b) noexcept {
  return a.string_view() <=> b.string_view();
}

// for libfmt 10+
template <std::size_t N, bool Z>
constexpr std::string_view format_as(
    const basic_short_string<N, Z>& s) noexcept {
  return std::string_view(s);
}

// Selected specializations
#ifdef __AVX2__
constexpr bool operator==(const short_nzstring<31>& a,
                          const short_nzstring<31>& b) noexcept {
  if consteval {
    return a.string_view() == b.string_view();
  } else {
    uint8_t l = a.size();
    __m256i va = *(const __m256i_u*)a.data();
    __m256i vb = *(const __m256i_u*)b.data();
    uint32_t ne = ~_mm256_movemask_epi8(_mm256_cmpeq_epi8(va, vb)) &
                  (0x80000000u | ((1u << l) - 1));
    return ne == 0;
  }
}

constexpr int strcmp_length_first_impl(const short_nzstring<31>& a,
                                       const short_nzstring<31>& b) noexcept {
  if (a.size() < b.size()) {
    return -1;
  } else if (a.size() > b.size()) {
    return 1;
  } else if consteval {
    return std::char_traits<char>::compare(a.data(), b.data(), a.size());
  } else {
    uint8_t l = a.size();
    __m256i va = *(const __m256i_u*)a.data();
    __m256i vb = *(const __m256i_u*)b.data();
    uint32_t mask = (1u << l) - 1;
    uint32_t ne = ~_mm256_movemask_epi8(_mm256_cmpeq_epi8(va, vb)) & mask;
    uint32_t lt = _mm256_movemask_epi8(__m256i(__v32qu(va) < __v32qu(vb)));
    uint32_t bit = ne & -ne;  // First different position
    return (lt & bit) ? -1 : bit ? 1 : 0;
  }
}
#endif

}  // namespace cbu
