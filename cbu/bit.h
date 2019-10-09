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

#include <cstdint>
#include <iterator>
#include <type_traits>
#include "utility.h"

namespace cbu {
inline namespace cbu_bit {


template <unsigned> struct UIntTypeByBitsTraits;

template <unsigned N> requires (N <= 8)
struct UIntTypeByBitsTraits<N> {
  using type = std::uint8_t;
};

template <unsigned N> requires (8 < N && N <= 16)
struct UIntTypeByBitsTraits<N> {
  using type = std::uint16_t;
};

template <unsigned N> requires (16 < N && N <= 32)
struct UIntTypeByBitsTraits<N> {
  using type = std::uint32_t;
};

template <unsigned N> requires (32 < N && N <= 64)
struct UIntTypeByBitsTraits<N> {
  using type = std::uint64_t;
};

template <unsigned N> requires (64 < N && N <= sizeof(std::uintmax_t) * 8)
struct UIntTypeByBitsTraits<N> {
  using type = std::uintmax_t;
};

template <unsigned N>
using UIntTypeByBits = typename UIntTypeByBitsTraits<N>::type;

template <unsigned N>
using IntTypeByBits = std::make_signed_t<UIntTypeByBits<N>>;

template <unsigned N>
using UIntTypeByBytes = UIntTypeByBits<N * 8>;

template <unsigned N>
using IntTypeByBytes = IntTypeByBits<N * 8>;

template <unsigned N>
using FastUIntTypeByBits = UIntTypeByBits<(N <= 32 ? 32 : N)>;

template <unsigned N>
using FastIntTypeByBits = std::make_signed_t<FastUIntTypeByBits<N>>;

template <unsigned N>
using FastUIntTypeByBytes = FastUIntTypeByBits<N * 8>;

template <unsigned N>
using FastIntTypeByBytes = FastIntTypeByBits<N * 8>;

static_assert(std::is_same_v<UIntTypeByBits<8>, std::uint8_t>);
static_assert(std::is_same_v<FastUIntTypeByBits<8>, std::uint32_t>);
static_assert(std::is_same_v<UIntTypeByBits<32>, std::uint32_t>);
static_assert(std::is_same_v<FastUIntTypeByBits<32>, std::uint32_t>);

// IMPORTANT: The following ctz/clz/bsr functions requires argument to be non-zero!!!
inline constexpr unsigned ctz(unsigned x) noexcept {
  return __builtin_ctz(x);
}

inline constexpr unsigned ctz(unsigned long x) noexcept {
  return __builtin_ctzl(x);
}

inline constexpr unsigned ctz(unsigned long long x) noexcept {
  return __builtin_ctzll(x);
}

inline constexpr unsigned ctz(int x) noexcept {
  return __builtin_ctz(x);
}

inline constexpr unsigned ctz(long x) noexcept {
  return __builtin_ctzl(x);
}

inline constexpr unsigned ctz(long long x) noexcept {
  return __builtin_ctzll(x);
}

inline constexpr unsigned clz(unsigned x) noexcept {
  return __builtin_clz(x);
}

inline constexpr unsigned clz(unsigned long x) noexcept {
  return __builtin_clzl(x);
}

inline constexpr unsigned clz(unsigned long long x) noexcept {
  return __builtin_clzll(x);
}

inline constexpr unsigned clz(int x) noexcept {
  return __builtin_clz(x);
}

inline constexpr unsigned clz(long x) noexcept {
  return __builtin_clzl(x);
}

inline constexpr unsigned clz(long long x) noexcept {
  return __builtin_clzll(x);
}

inline constexpr unsigned bsr(unsigned x) noexcept {
#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
  return __builtin_ia32_bsrsi(x);
#else
  return clz(x) ^ (8 * sizeof(x) - 1);
#endif
}

inline constexpr unsigned bsr(unsigned long x) noexcept {
#if defined __GNUC__ && defined __x86_64__
  return (sizeof(unsigned long) == 4 ? __builtin_ia32_bsrsi(x) :
          __builtin_ia32_bsrdi(x));
#else
  return clz(x) ^ (8 * sizeof(x) - 1);
#endif
}

inline constexpr unsigned bsr(unsigned long long x) noexcept {
#if defined __GNUC__ && defined __x86_64__
  return __builtin_ia32_bsrdi(x);
#else
  return clz(x) ^ (8 * sizeof(x) - 1);
#endif
}

inline constexpr unsigned bsr(int x) noexcept {
  return bsr(static_cast<unsigned>(x));
}

inline constexpr unsigned bsr(long x) noexcept {
  return bsr(static_cast<unsigned long>(x));
}

inline constexpr unsigned bsr(long long x) noexcept {
  return bsr(static_cast<unsigned long long>(x));
}

inline constexpr unsigned popcnt(unsigned x) noexcept {
  return __builtin_popcount(x);
}

inline constexpr unsigned popcnt(unsigned long x) noexcept {
  return __builtin_popcountl(x);
}

inline constexpr unsigned popcnt(unsigned long long x) noexcept {
  return __builtin_popcountll(x);
}

inline constexpr unsigned popcnt(int x) noexcept {
  return __builtin_popcount(x);
}

inline constexpr unsigned popcnt(long x) noexcept {
  return __builtin_popcountl(x);
}

inline constexpr unsigned popcnt(long long x) noexcept {
  return __builtin_popcountll(x);
}

template <typename T> requires std::is_unsigned<T>::value
inline constexpr T blsr(T x) noexcept { return (x & (x - 1)); }

template <typename T> requires (std::is_integral<T>::value && sizeof(T) == 4)
inline constexpr T bzhi(T x, unsigned k) noexcept {
#if defined __BMI2__
  return __builtin_ia32_bzhi_si(x, k);
#else
  return x & ((1u << k) - 1);
#endif
}

template <typename T> requires (std::is_integral<T>::value && sizeof(T) == 8)
inline constexpr T bzhi(T x, unsigned k) noexcept {
#if defined __x86_64__ && defined __BMI2__
  return __builtin_ia32_bzhi_di(x, k);
#else
  return x & ((uint64_t(1) << k) - 1);
#endif
}


// An iterator to iterate over set bits
template <typename T> requires std::is_unsigned<T>::value
class BitIterator : public std::iterator<std::forward_iterator_tag, unsigned> {
public:
  explicit constexpr BitIterator(T v = 0) : v_(v) {}
  BitIterator(const BitIterator &) = default;
  BitIterator &operator = (const BitIterator &) = default;

  unsigned operator * () const {
    return ctz(std::common_type_t<T, unsigned>(v_));
  }

  BitIterator &operator ++ () {
    v_ = blsr(v_); return *this;
  }
  BitIterator operator ++ (int) {
    return BitIterator(std::exchange(v_, blsr(v_)));
  }

  bool operator == (const BitIterator &o) const { return (v_ == o.v_); }
  bool operator != (const BitIterator &o) const { return (v_ != o.v_); }

private:
  T v_;
};

template <typename T> requires std::is_integral<T>::value
inline constexpr
cbu_utility::IteratorRange<BitIterator<std::make_unsigned_t<T>>>
set_bits(T v) {
  using UT = std::make_unsigned_t<T>;
  return {BitIterator<UT>(v), BitIterator<UT>(0)};
}

} // inline namespace cbu_bit
} // namespace cbu
