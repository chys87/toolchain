/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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
#include <cstdint>
#include <cstring>
#include <utility>
#include "byteorder.h"
#include "concepts.h"

namespace cbu {
inline namespace cbu_faststr {

// We could use void pointers, but we choose char to enforce stricter
// type checking
template <std::endian byte_order, Bswappable T, Raw_char_type C>
inline T mempick_bswap(const C *s) noexcept {
  T u;
  std::memcpy(std::addressof(u), s, sizeof(T));
  return bswap_for<byte_order>(u);
}

template <std::endian byte_order, Bswappable T, Raw_char_type C>
inline C *memdrop_bswap(C *s, T v) noexcept {
  T u = bswap_for<byte_order>(v);
  std::memcpy(s, std::addressof(u), sizeof(T));
  return (s + sizeof(T));
}

template <Bswappable T, Raw_char_type C>
inline T mempick(const C *s) noexcept {
  return mempick_bswap<std::endian::native, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline T mempick_le(const C *s) noexcept {
  return mempick_bswap<std::endian::little, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline T mempick_be(const C *s) noexcept {
  return mempick_bswap<std::endian::big, T>(s);
}

template <Bswappable T, Raw_char_type C>
inline C *memdrop(C *s, T v) noexcept {
  return memdrop_bswap<std::endian::native, T>(s, v);
}

template <Bswappable T, Raw_char_type C>
inline C *memdrop_le(C *s, T v) noexcept {
  return memdrop_bswap<std::endian::little, T>(s, v);
}

template <Bswappable T, Raw_char_type C>
inline C *memdrop_be(C *s, T v) noexcept {
  return memdrop_bswap<std::endian::big, T>(s, v);
}

template <Raw_char_type C>
inline std::uint16_t mempick2(const C *s) noexcept {
  return mempick<std::uint16_t>(s);
}

template <Raw_char_type C>
inline std::uint32_t mempick4(const C *s) noexcept {
  return mempick<std::uint32_t>(s);
}

template <Raw_char_type C>
inline std::uint64_t mempick8(const C *s) noexcept {
  return mempick<std::uint64_t>(s);
}

template <Raw_char_type C>
inline C *memdrop2(C *s, std::uint16_t v) noexcept {
  return memdrop(s, v);
}

template <Raw_char_type C>
inline C *memdrop4(C *s, std::uint32_t v) noexcept {
  return memdrop(s, v);
}

template <Raw_char_type C>
inline C *memdrop8(C *s, std::uint64_t v) noexcept {
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
  return static_cast<T *>(::memrchr(dst, c, n));
}

// Nonstandard
template <Raw_trivial_type T>
inline T *Mempset(T *dst, int c, std::size_t n) noexcept {
  return reinterpret_cast<T *>(
      reinterpret_cast<char *>(std::memset(dst, c, n)) + n);
}

void *memdrop(void *dst, std::uint64_t, std::size_t) noexcept;

template <Raw_char_type T>
inline T *memdrop(T *dst, std::uint64_t v, std::size_t n) {
  return static_cast<T *>(memdrop(static_cast<void *>(dst), v, n));
}

} // namespace cbu_faststr
} // namespace cbu
