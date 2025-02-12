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

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#ifdef __ARM_NEON
# include <arm_neon.h>
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

#include "cbu/common/byteorder.h"
#include "cbu/common/concepts.h"
#include "cbu/common/stdhack.h"
#include "cbu/compat/compilers.h"
#include "cbu/compat/string.h"

namespace cbu {

// Useful for some SIMD operations
extern const char arch_linear_bytes64[64];

constexpr const char* kDigits99 =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

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
template <Raw_trivial_type_or_void T, Raw_trivial_type_or_void S>
inline constexpr T *Memcpy(T *dst, const S *src, std::size_t n) noexcept {
  if consteval {
    if constexpr (Raw_char_type<T> && Raw_char_type<S>) {
      for (std::size_t i = 0; i < n; ++i) dst[i] = src[i];
      return dst;
    }
  }
  return static_cast<T *>(std::memcpy(dst, src, n));
}

template <Raw_trivial_type_or_void T, Raw_trivial_type_or_void S>
inline constexpr T *Mempcpy(T *dst, const S *src, std::size_t n) noexcept {
  if consteval {
    if constexpr (Raw_char_type<T> && Raw_char_type<S>) {
      for (; n; --n) *dst++ = *src++;
      return dst;
    }
  }
  // It appears only x86-64 actually has optimized mempcpy
#if defined __GLIBC__ && defined __x86_64__
  return static_cast<T *>(mempcpy(dst, src, n));
#else
  return reinterpret_cast<T *>(std::uintptr_t(std::memcpy(dst, src, n)) + n);
#endif
}

template <Raw_trivial_type_or_void T, Raw_trivial_type_or_void S>
inline constexpr T *Memmove(T *dst, const S *src, std::size_t n) noexcept {
  if consteval {
    if constexpr (Raw_char_type<T> && Raw_char_type<S>) {
      char* tmp = new char[n];
      Memcpy(tmp, src, n);
      Memcpy(dst, tmp, n);
      delete[] tmp;
      return dst;
    }
  }
  return static_cast<T *>(std::memmove(dst, src, n));
}

template <Raw_trivial_type_or_void T>
inline constexpr T *Memset(T *dst, int c, std::size_t n) noexcept {
  if consteval {
    if constexpr (Raw_char_type<T>) {
      for (std::size_t i = 0; i < n; ++i) dst[i] = T(c);
      return dst;
    }
  }
  return static_cast<T *>(std::memset(dst, c, n));
}

template <Char_type T>
inline constexpr T *Memchr(T *dst, int c, std::size_t n) noexcept {
  if consteval {
    for (std::size_t i = 0; i < n; ++i)
      if (dst[i] == T(c)) return dst + i;
    return nullptr;
  }
  return static_cast<T *>(std::memchr(dst, c, n));
}

template <Char_type T>
inline constexpr T *Memrchr(T *dst, int c, std::size_t n) noexcept {
  if consteval {
    for (std::size_t i = n; i; --i)
      if (dst[i - 1] == T(c)) return dst + i - 1;
    return nullptr;
  }
  return static_cast<T *>(compat::memrchr(dst, c, n));
}

// Nonstandard
template <Raw_trivial_type_or_void T>
inline constexpr T *Mempset(T *dst, int c, std::size_t n) noexcept {
  if consteval {
    if constexpr (Raw_char_type<T>) {
      return Memset(dst, c, n) + n;
    }
  }
  return reinterpret_cast<T *>(
      reinterpret_cast<char *>(std::memset(dst, c, n)) + n);
}

// They call the respective libc functions, ensuring no builtin replacement
// occurs, and ensure that the code generator actually uses the return value
// instead of remembering the first parameter.
namespace faststr_no_builtin_detail {

void* Memset(void*, int, std::size_t) noexcept;
void* Memcpy(void*, const void*, std::size_t) noexcept;
#if defined __GLIBC__ && defined __USE_GNU
void* Mempcpy(void*, const void*, std::size_t) noexcept;
#endif

}  // namespace faststr_no_builtin_detail

template <Raw_trivial_type_or_void T>
inline T* memset_no_builtin(T* p, int c, std::size_t n) noexcept {
  // Special-case mops, unfortunately GCC has no such macro
#ifdef __ARM_FEATURE_MOPS
  return Memset(p, c, n);
#else
  return static_cast<T*>(faststr_no_builtin_detail::Memset(p, c, n));
#endif
}

template <Raw_trivial_type_or_void T>
inline T* memcpy_no_builtin(T* d, const void* s, size_t n) noexcept {
#ifdef __ARM_FEATURE_MOPS
  return Memcpy(d, s, n);
#else
  return static_cast<T*>(faststr_no_builtin_detail::Memcpy(d, s, n));
#endif
}

template <Raw_trivial_type_or_void T>
inline T* mempcpy_no_builtin(T* d, const void* s, size_t n) noexcept {
  // It appears only x86-64 actually has optimized mempcpy
#if defined __GLIBC__ && defined __x86_64__
  return static_cast<T*>(faststr_no_builtin_detail::Mempcpy(d, s, n));
#else
  return reinterpret_cast<T*>(std::uintptr_t(memcpy_no_builtin(d, s, n)) + n);
#endif
}

CBU_AARCH64_PRESERVE_ALL void* memdrop_var32(void* dst, std::uint32_t,
                                             std::size_t) noexcept;
CBU_AARCH64_PRESERVE_ALL void* memdrop_var64(void* dst, std::uint64_t,
                                             std::size_t) noexcept;
#if __WORDSIZE >= 64
CBU_AARCH64_PRESERVE_ALL void* memdrop_var128(void* dst, unsigned __int128,
                                              std::size_t) noexcept;
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
    if constexpr (sizeof(IT) > 8)
      return static_cast<T*>(memdrop_var128(d, v, n));
    else
#endif
        if constexpr (sizeof(IT) > 4)
      return static_cast<T*>(memdrop_var64(d, v, n));
    else
      return static_cast<T*>(memdrop_var32(d, v, n));
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

}  // namespace cbu
