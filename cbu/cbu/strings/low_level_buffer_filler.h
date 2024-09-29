/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2024, chys <admin@CHYS.INFO>
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
#include <cstddef>
#include <cstring>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "cbu/common/byteorder.h"
#include "cbu/common/concepts.h"
#include "cbu/math/common.h"
#include "cbu/math/fastdiv.h"
#include "cbu/strings/faststr.h"
#include "cbu/strings/hex.h"

namespace cbu {

// min_size(), max_size(), static_max_size() functions are for str_builder

template <std::integral T, std::endian Order>
struct FillByEndian {
  T value;

  constexpr FillByEndian(T v) noexcept: value(v) {}

  template <Raw_char_type Ch>
  constexpr Ch* operator()(Ch* p) const noexcept {
    return memdrop_bswap<Order>(p, value);
  }

  static constexpr std::size_t min_size() noexcept { return sizeof(T); }
  static constexpr std::size_t max_size() noexcept { return sizeof(T); }
  static constexpr std::size_t static_max_size() noexcept { return sizeof(T); }
};

template <std::integral T>
using FillLittleEndian = FillByEndian<T, std::endian::little>;
template <std::integral T>
using FillBigEndian = FillByEndian<T, std::endian::big>;
template <std::integral T>
using FillNativeEndian = FillByEndian<T, std::endian::native>;

template <unsigned Width = 0, char Fill = '0'>
struct FillOptions {
  static inline constexpr unsigned width = Width;
  static inline constexpr char fill = Fill;

  template <unsigned NewWidth>
  using with_width = FillOptions<NewWidth, Fill>;

  template <char NewFill>
  using with_fill = FillOptions<Width, NewFill>;
};

// FillDec converts an unsigned number to a decimal string.
// The generated code is aggressively unrolled so it can be fairly bloated.
template <std::uint64_t UpperBound, typename Options = FillOptions<>>
struct FillDec;

template <std::uint64_t UpperBound, typename Options>
  requires(Options::width > 0)
struct FillDec<UpperBound, Options> {
  std::uint64_t value;

  constexpr FillDec(std::uint64_t v) noexcept : value(v) {}

  template <Raw_char_type Ch>
  constexpr Ch* operator()(Ch* p) const noexcept {
    conv_fixed_digit<UpperBound, Options::width>(value, p);
    return p + Options::width;
  }

  static constexpr std::size_t min_size() noexcept { return Options::width; }
  static constexpr std::size_t max_size() noexcept { return Options::width; }
  static constexpr std::size_t static_max_size() noexcept {
    return Options::width;
  }

  template <std::uint64_t UB, unsigned W, Raw_char_type Ch>
  static constexpr void conv_fixed_digit(std::uint64_t v, Ch* p) noexcept {
    if constexpr (Options::fill != '0' && W < Options::width) {
      if (v == 0) {
        for (unsigned k = W; k; --k) p[k - 1] = Options::fill;
        return;
      }
    }

    if constexpr (W >= 2 && Options::fill == '0' && UB >= 11) {
      std::uint64_t quot;
      std::uint64_t rem;
      if constexpr (UB == 0) {
        quot = v / 100;
        rem = v % 100;
      } else {
        quot = cbu::fastdiv<100, UB>(v);
        rem = cbu::fastmod<100, UB>(v);
      }
      const char* __restrict dg = kDigits99 + 2 * rem;
      memdrop2(p + W - 2, mempick2(dg));

      constexpr std::uint64_t NextUB =
          UB == 0 ? std::uint64_t(-1) / 100 + 1 : (UB - 1) / 100 + 1;
      conv_fixed_digit<NextUB, W - 2>(quot, p);
      return;
    } else if constexpr (W == 2 && UB >= 11 && UB <= 100) {
      // A common case, accelerate it
      const char* __restrict dg = kDigits99 + 2 * v;
      uint32_t chars = mempick2(dg);
      if (v < 10) {
        if constexpr (Options::fill > '0')
          chars += cbu::bswap_le<uint16_t>(Options::fill - '0');
        else
          chars -= cbu::bswap_le<uint16_t>('0' - Options::fill);
      }
      memdrop2(p, chars);
      return;
    } else {
      std::uint64_t quot;
      std::uint32_t rem;
      if constexpr (UB == 0) {
        quot = v / 10;
        rem = v % 10;
      } else {
        quot = cbu::fastdiv<10, UB>(v);
        rem = cbu::fastmod<10, UB>(v);
      }

      if constexpr (W <= 1) {
        if constexpr (W == 1) p[0] = Ch(rem + '0');
      } else {
        p[W - 1] = Ch(rem + '0');
        constexpr std::uint64_t NextUB =
            UB == 0 ? std::uint64_t(-1) / 10 + 1 : (UB - 1) / 10 + 1;
        conv_fixed_digit<NextUB, W - 1>(quot, p);
      }
    }
  }
};

template <std::uint64_t UpperBound, typename Options>
  requires(Options::width == 0)
struct FillDec<UpperBound, Options> {
  std::uint64_t value;
  std::uint8_t bytes;

  constexpr FillDec(std::uint64_t v) noexcept
      : value(v), bytes(get_bytes(v)) {}

  template <Raw_char_type Ch>
  constexpr Ch* operator()(Ch* p) const noexcept {
    return conv_flexible_digit(value, bytes, p);
  }

  static constexpr std::size_t static_max_size() noexcept {
    // This is correct for UpperBound == 0 as well
    return ilog10(std::uint64_t(UpperBound - 1)) + 1;
  }
  constexpr std::size_t max_size() const noexcept { return bytes; }
  constexpr std::size_t min_size() const noexcept { return bytes; }

  constexpr unsigned get_bytes(std::uint64_t v) const noexcept {
    if constexpr (0 < UpperBound && UpperBound <= 10) {
      return 1;
    } else if constexpr (10 < UpperBound && UpperBound <= 100) {
      return (v >= 10) + 1;
    } else if constexpr (100 < UpperBound && UpperBound <= 1000) {
      return (v >= 100) + (v >= 10) + 1;
    } else if constexpr (1000 < UpperBound && UpperBound <= 0xffffffff) {
      return ilog10(std::uint32_t(v)) + 1;
    } else {
      return ilog10(v) + 1;
    }
  }

  template <Raw_char_type Ch>
  static constexpr Ch* conv_flexible_digit(std::uint64_t v, unsigned b,
                                           Ch* p) noexcept {
    Ch* res = p + b;
    Ch* w = p + b - 1;
    if constexpr (UpperBound == 0) {
#ifdef __clang__
#pragma clang loop vectorize(disable) unroll(disable)
#endif
      do {
        *w-- = Ch(v % 10 + '0');
        v /= 10;
      } while (--b);
    } else if constexpr (UpperBound <= 10) {
      *p++ = Ch(v + '0');
    } else if constexpr (UpperBound <= 20) {
      if (b == 1) {
        *p++ = Ch(v + '0');
      } else {
        *p++ = '1';
        *p++ = Ch(v - 10 + '0');
      }
    } else if constexpr (UpperBound <= 100) {
      if (b == 1) {
        *p++ = Ch(v + '0');
      } else {
        p = cbu::memdrop2(p, cbu::mempick2(kDigits99 + 2 * v));
      }
    } else if constexpr (UpperBound <= 1000) {
      if (b >= 2) {
        std::uint32_t t = cbu::fastmod<100, UpperBound>(v);
        v = cbu::fastdiv<100, UpperBound>(v);
        cbu::memdrop2(w - 1, cbu::mempick2(kDigits99 + 2 * t));
        w -= 2;
      }
      if (b & 1) *w-- = '0' + v;
    } else {
#ifdef __clang__
#pragma clang loop vectorize(disable) unroll(disable)
#endif
      while (b >= 2) {
        b -= 2;
        std::uint32_t t = cbu::fastmod<100, UpperBound>(v);
        v = cbu::fastdiv<100, UpperBound>(v);
        cbu::memdrop2(w - 1, cbu::mempick2(kDigits99 + 2 * t));
        w -= 2;
      }
      if (b) *w-- = '0' + v;
    }
    return res;
  }
};

template <typename T>
FillDec(T) -> FillDec<
    (sizeof(T) >= 8 ? 0 : std::make_unsigned_t<T>(-1) + std::uint64_t(1))>;

// Write a string in hexadecimal form.  Currently only supports full width
template <Raw_unsigned_integral T>
struct FillHex {
  T value;

  static constexpr std::size_t static_max_size() noexcept {
    return 2 * sizeof(T);
  }
  static constexpr std::size_t max_size() noexcept { return 2 * sizeof(T); }
  static constexpr std::size_t min_size() noexcept { return 2 * sizeof(T); }
  constexpr char* operator()(char* w) const noexcept { return ToHex(w, value); }
};

template <Raw_unsigned_integral T>
FillHex(T) -> FillHex<T>;

template <std::ptrdiff_t diff>
struct FillSkipImpl {
  template <typename Ch>
  constexpr Ch* operator()(Ch* p) const noexcept {
    return p + diff;
  }
  static constexpr std::size_t min_size() noexcept { return diff; }
  static constexpr std::size_t max_size() noexcept { return diff; }
  static constexpr std::size_t static_max_size() noexcept { return diff; }
};

template <std::ptrdiff_t diff>
inline constexpr auto FillSkip = FillSkipImpl<diff>{};

template <Raw_char_type Ch>
struct FillGetPointer {
  Ch** ptr;

  explicit constexpr FillGetPointer(Ch** p) noexcept : ptr(p) {}

  constexpr Ch* operator()(Ch* p) const noexcept {
    *ptr = p;
    return p;
  }
  static constexpr std::size_t min_size() noexcept { return 0; }
  static constexpr std::size_t max_size() noexcept { return 0; }
  static constexpr std::size_t static_max_size() noexcept { return 0; }
};

template <Raw_char_type Ch>
FillGetPointer(Ch**) -> FillGetPointer<Ch>;

template <Raw_char_type Ch, Raw_integral Len>
struct FillGetLength {
  const Ch* start;
  Len* len;

  explicit constexpr FillGetLength(const Ch* start, Len* len) noexcept
      : start(start), len(len) {}
  constexpr Ch* operator()(Ch* p) const noexcept {
    *len = p - start;
    return p;
  }
  static constexpr std::size_t min_size() noexcept { return 0; }
  static constexpr std::size_t max_size() noexcept { return 0; }
  static constexpr std::size_t static_max_size() noexcept { return 0; }
};

// Low-level buffer filler
template <Char_type Ch>
class LowLevelBufferFiller {
 public:
  constexpr LowLevelBufferFiller() noexcept : p_(nullptr) {}
  explicit constexpr LowLevelBufferFiller(Ch* p) noexcept : p_(p) {}
  // Copy is safe, but makes no sense, so we disable it.
  LowLevelBufferFiller(const LowLevelBufferFiller&) = delete;
  constexpr LowLevelBufferFiller(LowLevelBufferFiller&& o) noexcept :
    p_(std::exchange(o.p_, nullptr)) {}

  LowLevelBufferFiller& operator=(const LowLevelBufferFiller&) = delete;
  constexpr LowLevelBufferFiller& operator=(LowLevelBufferFiller&& o) noexcept {
    swap(o);
    return *this;
  }

  constexpr void swap(LowLevelBufferFiller& o) noexcept { std::swap(p_, o.p_); }

  constexpr LowLevelBufferFiller& operator<<(Ch c) noexcept {
    *p_++ = c;
    return *this;
  }

  constexpr LowLevelBufferFiller& operator<<(
      std::basic_string_view<Ch> v) noexcept {
    auto size = v.size();
    if consteval {
      p_ = std::copy_n(v.data(), size, p_);
    } else {
      p_ = static_cast<Ch*>(std::memcpy(p_, v.data(), size)) + size;
    }
    return *this;
  }

  template <typename T>
  requires requires(T&& v) {
    { std::forward<T>(v)(std::declval<Ch*>()) } -> std::convertible_to<Ch*>;
  }
  constexpr LowLevelBufferFiller& operator<<(T&& v) noexcept {
    p_ = std::forward<T>(v)(p_);
    return *this;
  }

  template <typename Callback>
  requires requires(Callback&& cb, LowLevelBufferFiller& filler) {
    { std::forward<Callback>(cb)(filler) } -> std::convertible_to<Ch*>;
  }
  constexpr LowLevelBufferFiller& operator<<(Callback&& cb) noexcept(
      noexcept(cb(std::declval<LowLevelBufferFiller>()))) {
    p_ = std::forward<Callback>(cb)(*this);
    return *this;
  }

  template <typename... Args>
  constexpr LowLevelBufferFiller& operator<<(
      const std::tuple<Args...>& tpl) noexcept {
    std::apply([this](const Args&... args) { (*this << ... << args); }, tpl);
    return *this;
  }

  template <typename T>
    requires requires(const T& real, LowLevelBufferFiller& filler) {
      {filler << real} noexcept;
    }
  constexpr LowLevelBufferFiller& operator<<(
      const std::optional<T>& opt) noexcept {
    if (opt) *this << *opt;
    return *this;
  }

  constexpr Ch* pointer() const noexcept { return p_; }
  constexpr void set_pointer(Ch* p) noexcept { p_ = p; }

 private:
  Ch* p_;
};

} // namespace cbu
