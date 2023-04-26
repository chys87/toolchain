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

#include <bit>
#include <type_traits>

namespace cbu {

static_assert(
    std::endian::native == std::endian::little ||
      std::endian::native == std::endian::big,
    "Funny byte order.");

template <typename T>
concept Bswappable = (std::is_integral<T>::value &&
                      std::is_same_v<T, std::remove_cvref_t<T>> &&
                      !std::is_same_v<T, bool> &&
                      (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ||
                       sizeof(T) == 8 || sizeof(T) == 16));

template <Bswappable T> requires (sizeof(T) == 1)
inline constexpr T bswap(T x) noexcept { return x; }

template <Bswappable T> requires (sizeof(T) == 2)
inline constexpr T bswap(T x) noexcept { return __builtin_bswap16(x); }

template <Bswappable T> requires (sizeof(T) == 4)
inline constexpr T bswap(T x) noexcept { return __builtin_bswap32(x); }

template <Bswappable T> requires (sizeof(T) == 8)
inline constexpr T bswap(T x) noexcept { return __builtin_bswap64(x); }

template <Bswappable T> requires (sizeof(T) == 16)
inline constexpr T bswap(T x) noexcept {
  unsigned long long lo = static_cast<unsigned long long>(x);
  unsigned long long hi = static_cast<unsigned long long>(x >> 64);
  return (T(bswap(lo)) << 64) | bswap(hi);
}

template <std::endian order_a, std::endian order_b, Bswappable T>
  requires (order_a == order_b)
inline constexpr T may_bswap(T value) noexcept { return value; }

template <std::endian order_a, std::endian order_b, Bswappable T>
  requires (order_a != order_b)
inline constexpr T may_bswap(T value) noexcept { return bswap(value); }

template <Bswappable T>
inline constexpr T may_bswap(std::endian a, std::endian b, T value) noexcept {
  if (a == b) {
    return value;
  } else {
    return bswap(value);
  }
}

template <std::endian byte_order, Bswappable T>
inline constexpr T bswap_for(T value) noexcept {
  return may_bswap<byte_order, std::endian::native>(value);
}

template <Bswappable T>
inline constexpr T bswap_for(std::endian byte_order, T value) noexcept {
  return may_bswap(byte_order, std::endian::native, value);
}

// Swap for data exchange with big endian data
template <Bswappable T>
inline constexpr T bswap_be(T value) noexcept {
  return bswap_for<std::endian::big>(value);
}

// Swap for data exchange with little endian data
template <Bswappable T>
inline constexpr T bswap_le(T value) noexcept {
  return bswap_for<std::endian::little>(value);
}

template <Bswappable T, std::endian byte_order>
class [[gnu::packed]] PackedFixByteOrder {
 public:
  constexpr PackedFixByteOrder(T v) noexcept : v_(bswap_for<byte_order>(v)) {}
  constexpr PackedFixByteOrder(const PackedFixByteOrder &) noexcept = default;
  constexpr PackedFixByteOrder& operator=(const PackedFixByteOrder&) noexcept =
      default;

  constexpr operator T () const noexcept { return bswap_for<byte_order>(v_); }
  constexpr T operator+() const noexcept { return bswap_for<byte_order>(v_); }

  constexpr T raw_value() const noexcept { return v_; }

 private:
  T v_;
};

template <Bswappable T, std::endian byte_order>
class FixByteOrder {
 public:
  constexpr FixByteOrder() noexcept = default;
  constexpr FixByteOrder(T v) noexcept : v_(bswap_for<byte_order>(v)) {}
  constexpr FixByteOrder(const FixByteOrder &) noexcept = default;
  constexpr FixByteOrder& operator=(const FixByteOrder&) noexcept = default;

  constexpr operator T() const noexcept { return bswap_for<byte_order>(v_); }
  constexpr T operator+() const noexcept { return bswap_for<byte_order>(v_); }

  constexpr T raw_value() const noexcept { return v_; }

 private:
  T v_;
};

template <typename T, std::endian byte_order>
class FixByteOrderRef;

template <typename T, std::endian byte_order>
requires Bswappable<T>
class FixByteOrderRef<T, byte_order> {
 public:
  explicit constexpr FixByteOrderRef(T* p) noexcept : p_(p) {}
  constexpr FixByteOrderRef(const FixByteOrderRef&) noexcept = default;

  constexpr T load() const noexcept { return bswap_for<byte_order>(*p_); }
  constexpr void store(T v) noexcept { *p_ = bswap_for<byte_order>(v); }

  constexpr operator T() const noexcept { return load(); }
  constexpr FixByteOrderRef& operator=(T v) noexcept {
    store(v);
    return *this;
  }

  // Let it behave like real references (unlike std::reference_wrapper)
  constexpr FixByteOrderRef& operator=(const FixByteOrderRef& other) noexcept {
    store(other.load());
    return *this;
  }

 private:
  T* const p_;
};

template <typename T, std::endian byte_order>
requires Bswappable<T>
class FixByteOrderRef<const T, byte_order> {
 public:
  explicit constexpr FixByteOrderRef(const T* p) noexcept : p_(p) {}
  constexpr FixByteOrderRef(const FixByteOrderRef&) noexcept = default;

  constexpr T load() const noexcept { return bswap_for<byte_order>(*p_); }
  constexpr operator T() const noexcept { return load(); }
  constexpr FixByteOrderRef& operator=(const FixByteOrderRef&) = delete;

 private:
  const T* const p_;
};

template <Bswappable T> using PackedLittleEndian = PackedFixByteOrder<
    T, std::endian::little>;
template <Bswappable T> using PackedBigEndian = PackedFixByteOrder<
    T, std::endian::big>;
template <Bswappable T> using LittleEndian = FixByteOrder<
    T, std::endian::little>;
template <Bswappable T> using BigEndian = FixByteOrder<
    T, std::endian::big>;

// Template alias doesn't support deduction yet, so we define them as a
// function for now
template <typename T>
inline constexpr auto LittleEndianRef(T* p) noexcept {
  return FixByteOrderRef<T, std::endian::little>(p);
}

template <typename T>
inline constexpr auto BigEndianRef(T* p) noexcept {
  return FixByteOrderRef<T, std::endian::big>(p);
}

}  // namespace cbu
