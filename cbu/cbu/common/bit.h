/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

#include "cbu/common/utility.h"

namespace cbu {

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

namespace cbu_bit_detail {

template <typename T>
inline constexpr unsigned ctz_const(T x) noexcept {
  std::make_unsigned_t<T> y = x;
  unsigned r = 0;
  while ((y & 1) == 0) {
    y >>= 1;
    ++r;
  }
  return r;
}

template <typename T>
inline constexpr unsigned clz_const(T x) noexcept {
  using UT = std::make_unsigned_t<T>;
  UT y = x;
  unsigned r = 0;
  while ((y & (UT(1) << (8 * sizeof(T) - 1))) == 0) {
    y <<= 1;
    ++r;
  }
  return r;
}

}  // namespace cbu_bit_detail

// IMPORTANT: The following ctz/clz/bsr functions requires argument to be non-zero!!!
inline constexpr unsigned ctz(unsigned x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctz(x);
}

inline constexpr unsigned ctz(unsigned long x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctzl(x);
}

inline constexpr unsigned ctz(unsigned long long x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctzll(x);
}

inline constexpr unsigned ctz(int x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctz(x);
}

inline constexpr unsigned ctz(long x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctzl(x);
}

inline constexpr unsigned ctz(long long x) noexcept {
  if consteval {
    return cbu_bit_detail::ctz_const(x);
  }
  return __builtin_ctzll(x);
}

inline constexpr unsigned clz(unsigned x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clz(x);
}

inline constexpr unsigned clz(unsigned long x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clzl(x);
}

inline constexpr unsigned clz(unsigned long long x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clzll(x);
}

inline constexpr unsigned clz(int x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clz(x);
}

inline constexpr unsigned clz(long x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clzl(x);
}

inline constexpr unsigned clz(long long x) noexcept {
  if consteval {
    return cbu_bit_detail::clz_const(x);
  }
  return __builtin_clzll(x);
}

inline constexpr unsigned bsr(unsigned x) noexcept {
#if (defined __GNUC__ && (defined __i386__ || defined __x86_64__)) \
    && !defined __clang__
  if !consteval {
    return __builtin_ia32_bsrsi(x);
  }
#endif
  return clz(x) ^ (8 * sizeof(x) - 1);
}

inline constexpr unsigned bsr(unsigned long x) noexcept {
#if defined __GNUC__ && defined __x86_64__ && !defined __clang__
  if !consteval {
    return (sizeof(unsigned long) == 4 ? __builtin_ia32_bsrsi(x) :
            __builtin_ia32_bsrdi(x));
  }
#endif
  return clz(x) ^ (8 * sizeof(x) - 1);
}

inline constexpr unsigned bsr(unsigned long long x) noexcept {
#if defined __GNUC__ && defined __x86_64__ && !defined __clang__
  if !consteval {
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

template <typename T> requires std::is_integral_v<T>
inline constexpr T bzhi(T x, unsigned k) noexcept {
#if defined __BMI2__
  if !consteval {
    if constexpr (sizeof(T) <= 4)
      return __builtin_ia32_bzhi_si(std::make_unsigned_t<T>(x), k);
# if defined __x86_64__
    if constexpr (sizeof(T) == 8)
      return __builtin_ia32_bzhi_di(x, k);
# endif
  }
#endif
  if (k >= 8 * sizeof(T))
    return x;
  return x & ((T(1) << k) - 1);
}


// An iterator to iterate over set bits
template <std::unsigned_integral T>
class BitIterator {
 public:
  using value_type = unsigned int;
  using difference_type = int;
  using pointer = unsigned int*;
  using reference = unsigned int;
  using iterator_category = std::forward_iterator_tag;

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
inline constexpr IteratorRange<BitIterator<std::make_unsigned_t<T>>>
set_bits(T v) {
  using UT = std::make_unsigned_t<T>;
  return {BitIterator<UT>(v), BitIterator<UT>(0)};
}

static_assert(std::size(set_bits(0x123456789abcdef)) ==
              popcnt(0x123456789abcdef));

template <typename T> requires std::is_integral_v<T>
inline constexpr T pow2_floor(T n, std::type_identity_t<T> size) noexcept {
  return (n & ~(size - 1));
}

template <typename T> requires std::is_integral_v<T>
inline constexpr T pow2_ceil(T n, std::type_identity_t<T> size) noexcept {
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
  if (n <= 1) return 1;
  using UT = std::conditional_t<(sizeof(T) < sizeof(unsigned)), unsigned, T>;
  return (MAX_POW2<UT> >> (clz(UT(n - 1)) - 1));
}

// Flooring to the nearest power of 2
// Undefined for n == 0
template <typename T> requires std::is_integral_v<T>
inline constexpr T pow2_floor(T n) noexcept {
  using UT = std::conditional_t<(sizeof(T) < sizeof(unsigned)), unsigned, T>;
  return MAX_POW2<UT> >> clz(UT(n));
}

template <typename T> requires std::is_integral_v<T>
inline constexpr T is_pow2(T n) noexcept {
  return (n > 0 && (n & (n - 1)) == 0);
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
static_assert(pow2_ceil(uint16_t(9)) == 16);

static_assert(pow2_floor(1) == 1);
static_assert(pow2_floor(2) == 2);
static_assert(pow2_floor(3) == 2);
static_assert(pow2_floor(4) == 4);
static_assert(pow2_floor(7) == 4);
static_assert(pow2_floor(8) == 8);
static_assert(pow2_floor(15) == 8);
static_assert(pow2_floor(uint32_t(0xffff0f0f)) == 0x80000000);
static_assert(pow2_floor(uint16_t(15)) == 8);

static_assert(!is_pow2(0));
static_assert(is_pow2(1));
static_assert(is_pow2(2));
static_assert(!is_pow2(3));

template <typename T> requires (!std::is_enum_v<T> && std::is_signed_v<T>)
inline constexpr std::make_unsigned_t<T> as_unsigned(const T& x) noexcept {
  return x;
}

template <typename T> requires (!std::is_enum_v<T> && !std::is_signed_v<T>)
inline constexpr const T& as_unsigned(const T& x) noexcept { return x; }

template <typename T> requires std::is_enum_v<T>
inline constexpr auto as_unsigned(const T& x) noexcept {
  return static_cast<std::make_unsigned_t<std::underlying_type_t<T>>>(x);
}

namespace bit_mask_translate_detail {

template <typename Type>
struct NormalizeType {
  using type = std::make_unsigned_t<Type>;
};

template <typename Type> requires std::is_enum_v<Type>
struct NormalizeType<Type> {
  using type = std::make_unsigned_t<std::underlying_type_t<Type>>;
};

template <auto... TABLE>
struct TypeTraits;

template <auto FROM, auto TO>
struct TypeTraits<FROM, TO> {
  using FromType = typename NormalizeType<decltype(FROM)>::type;
  using ToType = typename NormalizeType<decltype(TO)>::type;
};

template <auto FROM, auto TO, auto FROM1, auto TO1, auto... MORE>
struct TypeTraits<FROM, TO, FROM1, TO1, MORE...> {
  using FromType =
      std::common_type_t<typename TypeTraits<FROM, TO>::FromType,
                         typename TypeTraits<FROM1, TO1, MORE...>::FromType>;
  using ToType =
      std::common_type_t<typename TypeTraits<FROM, TO>::ToType,
                         typename TypeTraits<FROM1, TO1, MORE...>::ToType>;
};

template <typename FromType, typename ToType, auto... TABLE>
struct IdentityMask;

template <typename FromType, typename ToType>
struct IdentityMask<FromType, ToType> {
  static constexpr ToType IDENTITY_MASK = ToType();
};

template <typename FromType, typename ToType, auto FROM, auto TO, auto... MORE>
struct IdentityMask<FromType, ToType, FROM, TO, MORE...> {
  static constexpr ToType IDENTITY_MASK =
      IdentityMask<FromType, ToType, MORE...>::IDENTITY_MASK |
      (as_unsigned(FROM) == as_unsigned(TO) ? as_unsigned(TO) : ToType());
};

template <typename FromType, typename ToType, auto... TABLE>
struct ResidualApply;

template <typename FromType, typename ToType>
struct ResidualApply<FromType, ToType> {
  static constexpr ToType apply(ToType res, FromType input) noexcept {
    return res;
  }
};

template <typename FromType, typename ToType, auto FROM, auto TO, auto... MORE>
struct ResidualApply<FromType, ToType, FROM, TO, MORE...> {
  static constexpr ToType apply(ToType res, FromType input) noexcept {
    constexpr auto UF = as_unsigned(FROM);
    constexpr auto UT = as_unsigned(TO);
    if constexpr (UF != UT) {
      if constexpr (is_pow2(UF) && is_pow2(UT)) {
        // TODO: This can be further optimized to combine many {FROM, TO}
        // with the same shift distances together
        if constexpr (UF > UT)
          res |= (input / (UF / UT)) & UT;
        else
          res |= (input * (UT / UF)) & UT;
      } else {
        if (input & UF) res |= UT;
      }
    }
    return ResidualApply<FromType, ToType, MORE...>::apply(res, input);
  }
};

static_assert(std::is_same_v<typename TypeTraits<1u, 2, 4l, short(8)>::FromType,
                             unsigned long>);
static_assert(std::is_same_v<typename TypeTraits<1u, 2, 4l, short(8)>::ToType,
                             unsigned int>);
static_assert(IdentityMask<unsigned, unsigned, 1, 1, 4, 8>::IDENTITY_MASK == 1);

}  // namespace bit_mask_translate_detail

template <auto... TABLE>
constexpr auto bit_mask_translate(
    typename bit_mask_translate_detail::TypeTraits<TABLE...>::FromType
        input) noexcept {
  using namespace bit_mask_translate_detail;
  using ToType = typename TypeTraits<TABLE...>::ToType;
  using FromType = typename TypeTraits<TABLE...>::FromType;
  ToType res =
      ToType(input & IdentityMask<FromType, ToType, TABLE...>::IDENTITY_MASK);
  return ResidualApply<FromType, ToType, TABLE...>::apply(res, input);
}

static_assert(bit_mask_translate<1, 1, 4, 8>(1) == 1);
static_assert(bit_mask_translate<1, 1, 4, 8>(0) == 0);
static_assert(bit_mask_translate<1, 1, 4, 8>(4) == 8);
static_assert(bit_mask_translate<1, 1, 4, 8>(5) == 9);
static_assert(bit_mask_translate<1, 1, 4, 8>(37) == 9);

}  // namespace cbu
