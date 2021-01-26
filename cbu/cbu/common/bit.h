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
#if (defined __GNUC__ && (defined __i386__ || defined __x86_64__)) \
    && !defined __clang__
  if (!std::is_constant_evaluated()) {
    return __builtin_ia32_bsrsi(x);
  }
#endif
  return clz(x) ^ (8 * sizeof(x) - 1);
}

inline constexpr unsigned bsr(unsigned long x) noexcept {
#if defined __GNUC__ && defined __x86_64__ && !defined __clang__
  if (!std::is_constant_evaluated()) {
    return (sizeof(unsigned long) == 4 ? __builtin_ia32_bsrsi(x) :
            __builtin_ia32_bsrdi(x));
  }
#endif
  return clz(x) ^ (8 * sizeof(x) - 1);
}

inline constexpr unsigned bsr(unsigned long long x) noexcept {
#if defined __GNUC__ && defined __x86_64__ && !defined __clang__
  if (!std::is_constant_evaluated()) {
    return __builtin_ia32_bsrdi(x);
  }
#endif
  return clz(x) ^ (8 * sizeof(x) - 1);
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

// popcnt: Count the number of set bits
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

// Clear lowest set bit
// Same as Intel BMI's BLSR instruction
template <typename T> requires std::is_integral<T>::value
inline constexpr T blsr(T x) noexcept { return (x & (x - 1)); }

// Isolate lowest set bit
// Same as Intel BMI's BLSI instruction
template <typename T> requires std::is_integral<T>::value
inline constexpr T blsi(T x) noexcept { return (x & -x); }

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
  return x & ((std::uint64_t(1) << k) - 1);
#endif
}


// An iterator to iterate over set bits
template <typename T> requires std::is_unsigned<T>::value
class BitIterator : public std::iterator<
    std::forward_iterator_tag, unsigned int, int, const unsigned int*,
    unsigned int
    // reference is not a real reference, as in istreambuf_iterator
    > {
public:
  explicit constexpr BitIterator(T v = 0) noexcept : v_(v) {}
  constexpr BitIterator(const BitIterator &) noexcept = default;
  constexpr BitIterator &operator = (const BitIterator &) noexcept = default;

  constexpr unsigned operator * () const noexcept {
    return ctz(std::common_type_t<T, unsigned>(v_));
  }

  constexpr BitIterator &operator ++ () {
    v_ = blsr(v_);
    return *this;
  }
  constexpr BitIterator operator ++ (int) {
    return BitIterator(std::exchange(v_, blsr(v_)));
  }

  constexpr bool operator == (const BitIterator &o) const noexcept {
    return (v_ == o.v_);
  }
  constexpr bool operator != (const BitIterator &o) const noexcept {
    return (v_ != o.v_);
  }

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

static_assert(std::size(set_bits(0x123456789abcdef)) ==
              popcnt(0x123456789abcdef));

inline constexpr std::size_t pow2_floor(std::size_t n,
                                        std::size_t size) noexcept {
  return (n & ~(size - 1));
}

inline constexpr std::size_t pow2_ceil(std::size_t n,
                                       std::size_t size) noexcept {
  return pow2_floor(n + size - 1, size);
}

template <typename T>
inline constexpr T *pow2_floor(T *p, std::size_t size) noexcept {
  return reinterpret_cast<T*>(std::uintptr_t(p) & ~std::uintptr_t(size - 1));
}

template <typename T>
inline constexpr T *pow2_ceil(T *p, std::size_t size) noexcept {
  return pow2_floor(reinterpret_cast<T *>(std::uintptr_t(p) + size - 1), size);
}

template <typename T> requires std::is_integral_v<T>
inline constexpr std::make_unsigned_t<T> MAX_POW2 =
    std::make_unsigned_t<T>(1) << (8 * sizeof(T) - 1);

// Ceiling to the nearest power of 2
// Undefined for n > MAX_POW2<T>
template <typename T> requires std::is_integral_v<T>
inline constexpr T pow2_ceil(T n) noexcept {
  return (MAX_POW2<T> >> (clz(std::max(n, T(1)) - 1) - 1));
}

// Flooring to the nearest power of 2
// Undefined for n == 0
template <typename T> requires std::is_integral_v<T>
inline constexpr T pow2_floor(T n) noexcept {
  return (std::make_unsigned_t<T>(1) << (8 * sizeof(T) - 1)) >> clz(n);
}

static_assert(pow2_ceil(0) == 1);
static_assert(pow2_ceil(1) == 1);
static_assert(pow2_ceil(2) == 2);
static_assert(pow2_ceil(3) == 4);
static_assert(pow2_ceil(4) == 4);
static_assert(pow2_ceil(5) == 8);
static_assert(pow2_ceil(8) == 8);
static_assert(pow2_ceil(9) == 16);
static_assert(pow2_ceil(uint32_t(0x80000000)) == 0x80000000);

static_assert(pow2_floor(1) == 1);
static_assert(pow2_floor(2) == 2);
static_assert(pow2_floor(3) == 2);
static_assert(pow2_floor(4) == 4);
static_assert(pow2_floor(7) == 4);
static_assert(pow2_floor(8) == 8);
static_assert(pow2_floor(15) == 8);
static_assert(pow2_floor(uint32_t(0xffff0f0f)) == 0x80000000);

} // inline namespace cbu_bit
} // namespace cbu
