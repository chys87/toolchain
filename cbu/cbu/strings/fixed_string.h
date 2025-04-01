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

#include <cstddef>
#include <span>
#include <string_view>

#include "cbu/common/tags.h"
#include "cbu/strings/zstring_view.h"

namespace cbu {

// The intended use of fixed_string is as template argument, see P0732R2
// (https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf), part
// of C++20.
// It can also be used to hold a run-time fixed-length string
template <std::size_t Len, bool HasTerminator = false>
class basic_fixed_string {
 public:
  explicit basic_fixed_string(UninitializedTag) noexcept {}

  constexpr basic_fixed_string() noexcept : s_{} {}

  consteval basic_fixed_string(const char (&s)[Len + 1]) noexcept : s_{} {
    assign(s);
  }

  constexpr basic_fixed_string(std::span<const char, Len> s) noexcept : s_{} {
    assign(s);
  }

  constexpr basic_fixed_string(UnsafeTag, std::string_view src) noexcept
      : s_{} {
    assign_unsafe(src);
  }

  constexpr char* data() noexcept { return s_; }
  constexpr const char* data() const noexcept { return s_; }
  constexpr const char* c_str() const noexcept requires HasTerminator {
    return s_;
  }
  constexpr char& operator[](std::size_t k) noexcept { return s_[k]; }
  constexpr const char& operator[](std::size_t k) const noexcept {
    return s_[k];
  }
  static constexpr size_t length() noexcept { return Len; }
  static constexpr size_t size() noexcept { return Len; }
  constexpr char* begin() noexcept { return s_; }
  constexpr char* end() noexcept { return s_ + Len; }
  constexpr const char* begin() const noexcept { return s_; }
  constexpr const char* end() const noexcept { return s_ + Len; }
  constexpr const char* cbegin() const noexcept { return s_; }
  constexpr const char* cend() const noexcept { return s_ + Len; }
  constexpr operator std::string_view() const noexcept { return {s_, Len}; }
  constexpr operator zstring_view() const noexcept requires HasTerminator {
    return {s_, Len};
  }

  constexpr void assign(std::span<const char, Len> src) noexcept {
    assign_unsafe(std::string_view(src.data(), src.size()));
  }
  constexpr void assign(const char (&s)[Len+1]) noexcept {
    assign_unsafe({s, Len});
  }

  constexpr void assign_unsafe(std::string_view src) noexcept {
    std::size_t copy_len = std::min(Len, src.size());
    std::copy_n(src.data(), copy_len, s_);
    std::fill_n(s_ + copy_len, sizeof(s_) - copy_len, 0);
  }

  // For some third-party type traits
  static constexpr inline bool pass_by_ref(basic_fixed_string*) {
    return true;
  }

  constexpr basic_fixed_string<Len, true> with_z() const noexcept {
    return basic_fixed_string<Len, true>(std::span(s_, Len));
  }

  constexpr basic_fixed_string<Len, true> without_z() const noexcept {
    return basic_fixed_string<Len, false>(std::span(s_, Len));
  }

  template <std::size_t Offset, std::size_t SubLen = Len - Offset>
  constexpr basic_fixed_string<std::min(SubLen, Len - Offset), true> substr()
      const noexcept {
    return {UnsafeTag(), std::string_view(s_ + Offset, std::min(SubLen, Len - Offset))};
  }

  struct StrBuilder {
    const char* s;

    static constexpr std::size_t static_max_size() noexcept { return Len; }
    static constexpr std::size_t static_min_size() noexcept { return Len; }
    constexpr std::size_t min_size() const noexcept { return Len; }
    constexpr std::size_t size() const noexcept { return Len; }
    constexpr char* write(char* w) const noexcept {
      if consteval {
        std::copy_n(s, Len, w);
      } else {
        std::memcpy(w, s, Len);
      }
      return w + Len;
    }
  };
  constexpr StrBuilder str_builder() const noexcept { return StrBuilder{s_}; }

  // Has to be public for basic_fixed_string to be used a s template argument
  char s_[Len + HasTerminator];
};

template <std::size_t N>
  requires(N > 0)
basic_fixed_string(const char (&)[N]) -> basic_fixed_string<N - 1, true>;

template <std::size_t N>
  requires(N > 0)
basic_fixed_string(const char (&)[N]) -> basic_fixed_string<N - 1, false>;

template <std::size_t N>
using fixed_string = basic_fixed_string<N, true>;

template <std::size_t N>
using fixed_nzstring = basic_fixed_string<N, false>;

template <std::size_t M, bool BM, std::size_t N, bool BN>
constexpr basic_fixed_string<M + N, BM || BN> operator+(
    basic_fixed_string<M, BM> a, basic_fixed_string<N, BN> b) noexcept {
  basic_fixed_string<M + N, BM || BN> res;
  for (std::size_t i = 0; i < M; ++i) res[i] = a[i];
  for (std::size_t i = 0; i < N; ++i) res[M + i] = b[i];
  if constexpr (BM || BN) res[M + N] = '\0';
  return res;
}

template <std::size_t M, std::size_t N, bool B>
  requires(M > 0)
constexpr auto operator+(const char (&a)[M],
                         basic_fixed_string<N, B> b) noexcept {
  return basic_fixed_string<M - 1, B>(a) + b;
}

template <std::size_t M, bool B, std::size_t N>
  requires(N > 0)
constexpr auto operator+(basic_fixed_string<M, B> a,
                         const char (&b)[N]) noexcept {
  return a + basic_fixed_string<N - 1, B>(b);
}

template <auto Value, bool HasTerminator = false>
  requires(Value >= 0)
consteval auto NumberToFixedString() noexcept {
  constexpr unsigned DIGITS = []() constexpr noexcept {
    auto v = Value;
    unsigned r = 0;
    do ++r;
    while (v /= 10);
    return r;
  }
  ();
  basic_fixed_string<DIGITS, HasTerminator> res;
  char* p = res.data() + DIGITS;
  auto v = Value;
  do *--p = '0' + (v % 10);
  while (v /= 10);
  return res;
}

template <auto Value, bool HasTerminator = false>
consteval auto LittleEndianFixedString() noexcept {
  basic_fixed_string<sizeof(Value), false> res;
  char* p = res.data();
  for (std::size_t i = 0; i < sizeof(Value); ++i)
    *p++ = static_cast<unsigned char>((Value >> (i * 8)) & 0xff);
  return res;
};

template <std::size_t Len, bool HasTerminator>
constexpr std::string_view format_as(
    const basic_fixed_string<Len, HasTerminator>& s) noexcept {
  return std::string_view(s);
}

}  // namespace cbu
