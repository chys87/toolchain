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

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif

#include <bit>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "byteorder.h"
#include "cbu/compat/string.h"
#include "concepts.h"
#include "stdhack.h"

namespace cbu {

// Useful for some SIMD operations
extern const char arch_linear_bytes64[64];

// We could use void pointers, but we choose char to enforce stricter
// type checking
template <std::endian byte_order, Bswappable T, Raw_char_type C>
inline constexpr T mempick_bswap(const C* s) noexcept {
  if consteval {
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
  if consteval {
    T u = may_bswap<std::endian::little, byte_order>(v);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      *s++ = std::uint8_t(u);
      // Don't write ">> 8".  That's UB if sizeof(T) == 1
      u = u >> 1 >> 7;
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
#ifdef __GLIBC__
  return static_cast<T *>(mempcpy(dst, src, n));
#else
  return reinterpret_cast<T *>(std::uintptr_t(std::memcpy(dst, src, n)) + n);
#endif
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

void* memdrop_var64(void* dst, std::uint64_t, std::size_t) noexcept;
#if __WORDSIZE >= 64
void* memdrop_var128(void* dst, unsigned __int128, std::size_t) noexcept;
#endif
#ifdef __SSE2__
void* memdrop(void* dst, __m128i, std::size_t) noexcept;
#endif
#ifdef __AVX2__
void* memdrop(void* dst, __m256i, std::size_t) noexcept;
#endif

template <typename T, typename IT>
requires ((Raw_char_type<T> || std::is_void_v<T>) &&
          (Raw_integral<IT>
#if __WORDSIZE >= 64
           || std::is_same_v<IT, unsigned __int128>
           || std::is_same_v<IT, __int128>
#endif
          ))
inline constexpr T* memdrop(T* dst, IT v, std::size_t n) noexcept {
  using S = std::conditional_t<std::is_void_v<T>, char, T>;
  S* d = static_cast<S*>(dst);
  if consteval {
    char buf[sizeof(v)];
    memdrop(buf, v);
    for (std::size_t i = 0; i < n; ++i) {
      d[i] = buf[i];
    }
    return d + n;
  } else {
#if __WORDSIZE >= 64
    if (sizeof(IT) > 8)
      return static_cast<T*>(memdrop_var128(d, v, n));
#endif
    return static_cast<T*>(memdrop_var64(d, v, n));
  }
}

#ifdef __SSE2__
template <Raw_char_type T>
inline T* memdrop(T* dst, __m128i v, std::size_t n) noexcept {
  return static_cast<T*>(memdrop(static_cast<void*>(dst), v, n));
}
#endif

#ifdef __AVX2__
template <Raw_char_type T>
inline T* memdrop(T* dst, __m256i v, std::size_t n) noexcept {
  return static_cast<T*>(memdrop(static_cast<void*>(dst), v, n));
}
#endif

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
inline void append(
    std::basic_string<C>* res,
    std::type_identity_t<std::initializer_list<std::basic_string_view<C>>> il) {
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

inline std::u8string concat(std::initializer_list<std::u8string_view> il) {
  return concat(std::span<const std::u8string_view>(il.begin(), il.size()));
}

// Convert a hexadecimal digit to number
inline constexpr std::optional<unsigned> convert_xdigit(
    std::uint8_t c, bool assume_valid = false) noexcept {
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  } else if (assume_valid || ((c | 0x20) >= 'a' && (c | 0x20) <= 'f')) {
    return (((c | 0x20) - 'a') + 10);
  } else {
    return std::nullopt;
  }
}

// Convert 2 hexadecimal digits
inline constexpr std::optional<unsigned> convert_2xdigit(
    const char *s, bool assume_valid = false) noexcept {
  auto a = convert_xdigit(*s++, assume_valid);
  if (!a) return a;
  auto b = convert_xdigit(*s++, assume_valid);
  if (!b) return b;
  return *a * 16 + *b;
}

// Convert 4 hexadecimal digits
inline constexpr std::optional<unsigned> convert_4xdigit(
    const char *s, bool assume_valid = false) noexcept {
#if defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(__v4su{mempick_be<uint32_t>(s), 0, 0, 0});
    __v16qu digits = (v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = (v_alpha < 6);
    if (!assume_valid &&
        !_mm_testc_si128(__m128i(digits | alpha), _mm_setr_epi32(-1, 0, 0, 0)))
      return std::nullopt;
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    uint32_t t = __v4su(res)[0];
    return _pext_u32(t, 0x0f0f0f0f);
  }
#endif
  auto a = convert_xdigit(*s++, assume_valid);
  if (!a) return a;
  auto b = convert_xdigit(*s++, assume_valid);
  if (!b) return b;
  auto c = convert_xdigit(*s++, assume_valid);
  if (!c) return c;
  auto d = convert_xdigit(*s++, assume_valid);
  if (!d) return d;
  return ((*a * 16 + *b) * 16 + *c) * 16 + *d;
}

// Convert 8 hexadecimal digits
inline constexpr std::optional<unsigned> convert_8xdigit(
    const char *s, bool assume_valid = false) noexcept {
#if defined __x86_64__ && defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(__v2du{mempick_be<uint64_t>(s), 0});
    __v16qu digits = (v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = (v_alpha < 6);
    if (!assume_valid &&
        !_mm_testc_si128(__m128i(digits | alpha), _mm_setr_epi32(-1, -1, 0, 0)))
      return std::nullopt;
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    uint64_t t = __v2du(res)[0];
    return unsigned(_pext_u64(t, 0x0f0f0f0f'0f0f0f0f));
  }
#endif
  auto a = convert_4xdigit(s, assume_valid);
  auto b = convert_4xdigit(s + 4, assume_valid);
  if (!a || !b) return std::nullopt;
  return (*a << 16) + *b;
}

// Convert 16 hexadecimal digits
inline constexpr std::optional<std::uint64_t> convert_16xdigit(
    const char *s, bool assume_valid = false) noexcept {
#if defined __x86_64__ && defined __SSE4_1__ && defined __BMI2__
  if !consteval {
    __v16qu v = __v16qu(_mm_shuffle_epi8(
          *(const __m128i_u*)s,
          _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)));
    __v16qu digits = (v - '0' <= 9);
    __v16qu v_alpha = (v | 0x20) - 'a';
    __v16qu alpha = (v_alpha < 6);
    if (!assume_valid &&
        !_mm_testc_si128(__m128i(digits | alpha), _mm_set1_epi8(-1)))
      return std::nullopt;
    __m128i res = _mm_blendv_epi8(
        __m128i(v_alpha + 10), __m128i(v - '0'), __m128i(digits));
    std::uint64_t lo = _pext_u64(__v2du(res)[0], 0x0f0f0f0f'0f0f0f0f);
    std::uint64_t hi = _pext_u64(__v2du(res)[1], 0x0f0f0f0f'0f0f0f0f);
    return (hi << 32) | lo;
  }
#endif
  auto a = convert_8xdigit(s, assume_valid);
  auto b = convert_8xdigit(s + 8, assume_valid);
  if (!a || !b) return std::nullopt;
  return (std::uint64_t(*a) << 32) | *b;
}

}  // namespace cbu
