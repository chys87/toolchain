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

#include <bit>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <string>
#include <utility>
#include "byteorder.h"
#include "concepts.h"
#include "stdhack.h"
#include "cbu/compat/string.h"
#include "cbu/compat/type_identity.h"

namespace cbu {
inline namespace cbu_faststr {

// We could use void pointers, but we choose char to enforce stricter
// type checking
template <std::endian byte_order, Bswappable T, Raw_char_type C>
inline constexpr T mempick_bswap(const C* s) noexcept {
  if (std::is_constant_evaluated()) {
    T u = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      u = (u << 8) | std::uint8_t(*s++);
    }
    return may_bswap<std::endian::big, byte_order>(u);
  } else {
    T u;
    std::memcpy(std::addressof(u), s, sizeof(T));
    return bswap_for<byte_order>(u);
  }
}

template <std::endian byte_order, Bswappable T, Raw_char_type C>
inline constexpr C* memdrop_bswap(C *s, T v) noexcept {
  if (std::is_constant_evaluated()) {
    T u = may_bswap<std::endian::little, byte_order>(v);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      *s++ = std::uint8_t(u);
      u >>= 8;
    }
    return s;
  } else {
    T u = bswap_for<byte_order>(v);
    std::memcpy(s, std::addressof(u), sizeof(T));
    return (s + sizeof(T));
  }
}

template <Bswappable T, Raw_char_type C>
inline constexpr T mempick(const C* s) noexcept {
  return mempick_bswap<std::endian::native, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline constexpr T mempick_le(const C* s) noexcept {
  return mempick_bswap<std::endian::little, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline constexpr T mempick_be(const C* s) noexcept {
  return mempick_bswap<std::endian::big, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline constexpr C* memdrop(C* s, T v) noexcept {
  return memdrop_bswap<std::endian::native, T>(s, v);
}

template <Bswappable T, Raw_char_type C>
inline constexpr C* memdrop_le(C* s, T v) noexcept {
  return memdrop_bswap<std::endian::little, T>(s, v);
}

template <Bswappable T, Raw_char_type C>
inline constexpr C* memdrop_be(C* s, T v) noexcept {
  return memdrop_bswap<std::endian::big, T>(s, v);
}

template <Raw_char_type C>
inline constexpr std::uint16_t mempick2(const C* s) noexcept {
  return mempick<std::uint16_t>(s);
}

template <Raw_char_type C>
inline constexpr std::uint32_t mempick4(const C* s) noexcept {
  return mempick<std::uint32_t>(s);
}

template <Raw_char_type C>
inline constexpr std::uint64_t mempick8(const C* s) noexcept {
  return mempick<std::uint64_t>(s);
}

template <Raw_char_type C>
inline constexpr C* memdrop2(C* s, std::uint16_t v) noexcept {
  return memdrop(s, v);
}

template <Raw_char_type C>
inline constexpr C* memdrop4(C* s, std::uint32_t v) noexcept {
  return memdrop(s, v);
}

template <Raw_char_type C>
inline constexpr C* memdrop8(C* s, std::uint64_t v) noexcept {
  return memdrop(s, v);
}


// For C++, define memory operations that return the proper types
template <Raw_trivial_type T>
inline T *Memcpy(T *dst, const void *src, std::size_t n) noexcept {
  return static_cast<T *>(std::memcpy(dst, src, n));
}

template <Raw_trivial_type T>
inline T *Mempcpy(T *dst, const void *src, std::size_t n) noexcept {
  return reinterpret_cast<T *>(std::uintptr_t(std::memcpy(dst, src, n)) + n);
}

template <Raw_trivial_type T>
inline T *Memmove(T *dst, const void *src, std::size_t n) noexcept {
  return static_cast<T *>(std::memmove(dst, src, n));
}

template <Raw_trivial_type T>
inline T *Memset(T *dst, int c, std::size_t n) noexcept {
  return static_cast<T *>(std::memset(dst, c, n));
}

template <Char_type T>
inline T *Memchr(T *dst, int c, std::size_t n) noexcept {
  return static_cast<T *>(std::memchr(dst, c, n));
}

template <Char_type T>
inline T *Memrchr(T *dst, int c, std::size_t n) noexcept {
  return static_cast<T *>(compat::memrchr(dst, c, n));
}

// Nonstandard
template <Raw_trivial_type T>
inline T *Mempset(T *dst, int c, std::size_t n) noexcept {
  return reinterpret_cast<T *>(
      reinterpret_cast<char *>(std::memset(dst, c, n)) + n);
}

void *memdrop(void *dst, std::uint64_t, std::size_t) noexcept;

template <Raw_char_type T>
inline constexpr T* memdrop(T* dst, std::uint64_t v, std::size_t n) noexcept {
  if (std::is_constant_evaluated()) {
    char buf[sizeof(v)];
    memdrop(buf, v);
    for (std::size_t i = 0; i < n; ++i) {
      dst[i] = buf[i];
    }
    return dst + n;
  } else {
    return static_cast<T *>(memdrop(static_cast<void *>(dst), v, n));
  }
}

// Use sprintf functions with std::string
std::size_t append_vnprintf(std::string* res, std::size_t hint_size,
                            const char* format, std::va_list ap)
  __attribute__((__format__(__printf__, 3, 0)));

std::size_t append_nprintf(std::string* res, std::size_t hint_size,
                           const char *format, ...)
  __attribute__((__format__(__printf__, 3, 4)));

std::string vnprintf(std::size_t hint_size,
                     const char* format, std::va_list ap)
  __attribute__((__format__(__printf__, 2, 0)));

std::string nprintf(std::size_t hint_size,
                    const char* format, ...)
  __attribute__((__format__(__printf__, 2, 3)));

// append
template <Std_string_char C>
void append(std::basic_string<C>* res,
            std::span<const std::basic_string_view<C>> sp) {
  std::size_t n = sp.size();
  auto* p = sp.data();
  if (n == 0) {
    // Add this special case so that GCC doesn't generate a spurious
    // call to extend in the empty branch.
    return;
  }

  // n == 2 is a common case - accelerate it
  if (n == 2) {
    auto& a = *p;
    auto& b = *(p + 1);
    std::size_t l = a.length() + b.length();
    C* w = extend(res, l);
    w = Mempcpy(w, a.data(), a.length() * sizeof(C));
    w = Mempcpy(w, b.data(), b.length() * sizeof(C));
    return;
  }

  std::size_t l = 0;
  auto q = p;
  for (auto k = n; k; --k) {
    l += (q++)->length();
  }
  C* w = extend(res, l);
  for (; n; --n) {
    auto sv = *p++;
    w = Mempcpy(w, sv.data(), sv.length() * sizeof(C));
  }
}

extern template void append<char>(std::string*,
                                  std::span<const std::string_view>);

template <Std_string_char C>
inline void append(std::basic_string<C>* res,
                   compat::type_identity_t<
                     std::initializer_list<std::basic_string_view<C>>> il) {
  append(res,
         std::span<const std::basic_string_view<C>>(il.begin(), il.size()));
}

template <Std_string_char C>
std::basic_string<C> concat(std::span<const std::basic_string_view<C>> sp) {
  std::basic_string<C> res;
  append(&res, sp);
  return res;
}

// enumerate all types, so that the user can use initializer list more easily
inline std::string concat(std::initializer_list<std::string_view> il) {
  return concat(std::span<const std::string_view>(il.begin(), il.size()));
}

inline std::wstring concat(std::initializer_list<std::wstring_view> il) {
  return concat(std::span<const std::wstring_view>(il.begin(), il.size()));
}

inline std::u16string concat(std::initializer_list<std::u16string_view> il) {
  return concat(std::span<const std::u16string_view>(il.begin(), il.size()));
}

inline std::u32string concat(std::initializer_list<std::u32string_view> il) {
  return concat(std::span<const std::u32string_view>(il.begin(), il.size()));
}

#if defined __cpp_char8_t && __cpp_char8_t >= 201811
inline std::u8string concat(std::initializer_list<std::u8string_view> il) {
  return concat(std::span<const std::u8string_view>(il.begin(), il.size()));
}
#endif

} // namespace cbu_faststr
} // namespace cbu
