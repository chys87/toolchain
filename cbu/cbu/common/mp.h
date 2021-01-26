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

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace cbu {
namespace mp {

#if __WORDSIZE == 64 || defined __x86_64__
using Word = std::uint64_t;
using DWord = unsigned __int128;
#else
using Word = std::uint32_t;
using DWord = std::uint64_t;
#endif

template <typename T>
inline constexpr std::size_t minimize(const T* s, std::size_t n) noexcept {
  while (n && s[n - 1] == 0)
    --n;
  return n;
}

template <bool NEED_MINIMIZE, typename T>
inline constexpr std::size_t may_minimize(const T* w, std::size_t n) noexcept {
  if constexpr (NEED_MINIMIZE)
    return minimize(w, n);
  else
    return n;
}

size_t mul(Word *r, const Word *a, size_t na, Word b) noexcept;
size_t mul(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept;
size_t add(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept;
size_t add(Word *r, const Word *a, size_t na, Word b) noexcept;
std::pair<size_t, Word> div(Word *r, const Word *a, size_t na, Word b) noexcept;

int compare(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool eq(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool ne(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool gt(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool lt(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool ge(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool le(const Word *a, size_t na, const Word *b, size_t nb) noexcept;

size_t from_dec(Word *r, const char *s, size_t n) noexcept;
char *to_dec(char *r, const Word *s, size_t n) noexcept;
std::string to_dec(const Word *s, size_t n) noexcept;

size_t from_hex(Word *r, const char *s, size_t n) noexcept;
char *to_hex(char *r, const Word *s, size_t n) noexcept;
std::string to_hex(const Word *s, size_t n) noexcept;

size_t from_oct(Word *r, const char *s, size_t n) noexcept;
char *to_oct(char *r, const Word *s, size_t n) noexcept;
std::string to_oct(const Word *s, size_t n) noexcept;

size_t from_bin(Word *r, const char *s, size_t n) noexcept;
char *to_bin(char *r, const Word *s, size_t n) noexcept;
std::string to_bin(const Word *s, size_t n) noexcept;

enum struct Radix {
  BIN = 2,
  OCT = 8,
  DEC = 10,
  HEX = 16,
};

template <Radix> struct StringConversion;
template <> struct StringConversion<Radix::BIN> {
  static std::size_t from_str(Word* r, std::string_view s) noexcept {
    return from_bin(r, s.data(), s.size());
  }
  static char* to_str(char* r, const Word* s, std::size_t n) noexcept {
    return to_bin(r, s, n);
  }
  static std::string to_str(const Word* s, std::size_t n) noexcept {
    return to_bin(s, n);
  }
};
template <> struct StringConversion<Radix::OCT> {
  static std::size_t from_str(Word* r, std::string_view s) noexcept {
    return from_oct(r, s.data(), s.size());
  }
  static char* to_str(char* r, const Word* s, std::size_t n) noexcept {
    return to_oct(r, s, n);
  }
  static std::string to_str(const Word* s, std::size_t n) noexcept {
    return to_oct(s, n);
  }
};
template <> struct StringConversion<Radix::DEC> {
  static std::size_t from_str(Word* r, std::string_view s) noexcept {
    return from_dec(r, s.data(), s.size());
  }
  static char* to_str(char* r, const Word* s, std::size_t n) noexcept {
    return to_dec(r, s, n);
  }
  static std::string to_str(const Word* s, std::size_t n) noexcept {
    return to_dec(s, n);
  }
};
template <> struct StringConversion<Radix::HEX> {
  static std::size_t from_str(Word* r, std::string_view s) noexcept {
    return from_hex(r, s.data(), s.size());
  }
  static char* to_str(char* r, const Word* s, std::size_t n) noexcept {
    return to_hex(r, s, n);
  }
  static std::string to_str(const Word* s, std::size_t n) noexcept {
    return to_hex(s, n);
  }
};

template <Radix radix>
struct Radixed {
  std::string_view s;
};

template <bool CONST, bool MINIMIZED>
class BasicRef {
 public:
  using W = std::conditional_t<CONST, const Word, Word>;

  constexpr BasicRef() noexcept : w_(nullptr), n_(0) {}
  constexpr BasicRef(Word* w, std::size_t n) noexcept :
    w_(w), n_(may_minimize<MINIMIZED>(w, n)) {}
  template <Radix radix> BasicRef(Word* w, Radixed<radix> rs) noexcept :
    BasicRef(w, StringConversion<radix>::from_str(w, rs.s)) {}

  template <bool OTHER_CONST, bool OTHER_MINIMIZED>
    requires (!OTHER_CONST || CONST)
  constexpr BasicRef(const BasicRef<OTHER_CONST, OTHER_MINIMIZED>& o) noexcept :
    w_(o.data()),
    n_(may_minimize<!OTHER_MINIMIZED && MINIMIZED>(o.data(), o.size())) {}

  constexpr BasicRef& operator=(const BasicRef&) noexcept = default;

  constexpr W* data() const noexcept { return w_; }
  constexpr std::size_t size() const noexcept { return n_; }

  constexpr BasicRef<CONST, true> minimize() const noexcept {
    if constexpr (MINIMIZED) {
      return *this;
    } else {
      return {w_, ::cbu::mp::minimize(w_, n_)};
    }
  }

  template <Radix radix> char* to_str(char* r) noexcept {
    return StringConversion<radix>::to_str(r, w_, n_);
  }
  template <Radix radix> std::string to_str() noexcept {
    return StringConversion<radix>::to_str(w_, n_);
  }

 private:
  W* w_;
  std::size_t n_;
};

using Ref = BasicRef<false, false>;
using MinRef = BasicRef<false, true>;
using ConstRef = BasicRef<true, false>;
using ConstMinRef = BasicRef<true, true>;

} // namespace mp
} // namespace cbu
