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
#include <type_traits>

namespace cbu {
inline namespace cbu_byteorder {

static_assert(
    std::endian::native == std::endian::little ||
      std::endian::native == std::endian::big,
    "Funny byte order.");

template <typename T>
concept Bswappable = std::is_integral<T>::value && sizeof(T) <= 8;

template <Bswappable T> requires sizeof(T) == 1
inline constexpr T bswap(T x) noexcept { return x; }

template <Bswappable T> requires sizeof(T) == 2
inline constexpr T bswap(T x) noexcept { return __builtin_bswap16(x); }

template <Bswappable T> requires sizeof(T) == 4
inline constexpr T bswap(T x) noexcept { return __builtin_bswap32(x); }

template <Bswappable T> requires sizeof(T) == 8
inline constexpr T bswap(T x) noexcept { return __builtin_bswap64(x); }

template <std::endian order_a, std::endian order_b, Bswappable T>
  requires order_a == order_b
inline constexpr T may_bswap(T value) noexcept { return value; }

template <std::endian order_a, std::endian order_b, Bswappable T>
  requires order_a != order_b
inline constexpr T may_bswap(T value) noexcept { return bswap(value); }

template <std::endian byte_order, Bswappable T>
inline constexpr T bswap_for(T value) noexcept {
  return may_bswap<byte_order, std::endian::native>(value);
}

// Swap for data exchange with big endian data
Bswappable{T}
inline constexpr T bswap_be(T value) noexcept {
  return bswap_for<std::endian::big>(value);
}

// Swap for data exchange with little endian data
Bswappable{T}
inline constexpr T bswap_le(T value) noexcept {
  return bswap_for<std::endian::little>(value);
}

template <Bswappable T, std::endian byte_order>
class [[gnu::packed]] PackedFixByteOrder {
 public:
  constexpr PackedFixByteOrder(T v) noexcept : v_(bswap_for<byte_order>(v)) {}
  PackedFixByteOrder(const PackedFixByteOrder &) noexcept = default;

  constexpr operator T () const noexcept { return bswap_for<byte_order>(v_); }

 private:
  T v_;
};

template <Bswappable T, std::endian byte_order>
class FixByteOrder {
 public:
  constexpr FixByteOrder(T v) noexcept : v_(bswap_for<byte_order>(v)) {}
  FixByteOrder(const FixByteOrder &) noexcept = default;

  constexpr operator T () const noexcept { return bswap_for<byte_order>(v_); }

 private:
  T v_;
};

template <Bswappable T> using PackedLittleEndian = PackedFixByteOrder<
    T, std::endian::little>;
template <Bswappable T> using PackedBigEndian = PackedFixByteOrder<
    T, std::endian::big>;
template <Bswappable T> using LittleEndian = FixByteOrder<
    T, std::endian::little>;
template <Bswappable T> using BigEndian = FixByteOrder<
    T, std::endian::big>;

} // inline namespace cbu_byteorder
} // namespace cbu
