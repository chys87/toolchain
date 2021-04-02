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

#include <compare>
#include <limits>
#include <utility>

#include "cbu/common/concepts.h"
#include "cbu/common/fastarith.h"

namespace cbu {

// DoubleUnsigned<T> behaves like an unsigned integer type with double the
// size of T.
// This is intended to be used in constexpr evaluations, without optimizing
// for non-constexpr use.
template <typename T>
struct DoubleUnsigned {
  T lo;
  T hi;

  static_assert(std::is_unsigned_v<T>);

  template <Raw_integral U = T>
  constexpr DoubleUnsigned(U x = 0) noexcept
      : lo(std::make_unsigned_t<U>(x) & std::numeric_limits<T>::max()),
        hi(std::make_unsigned_t<std::common_type_t<T, U>>(
               std::make_unsigned_t<U>(x)) >>
           (8 * sizeof(T) - 1) >> 1) {}
  constexpr DoubleUnsigned(T l, T h) noexcept : lo(l), hi(h) {}
  constexpr DoubleUnsigned(const DoubleUnsigned&) noexcept = default;
  constexpr DoubleUnsigned& operator=(const DoubleUnsigned&) noexcept = default;

  static constexpr DoubleUnsigned from_mul(T a, T b) noexcept {
    if constexpr (sizeof(T) == 1) {
      unsigned c = unsigned(a) * b;
      return {T(c), T(c / (unsigned(std::numeric_limits<T>::max()) + 1))};
    } else {
      constexpr unsigned kShift = 4 * sizeof(T);
      constexpr T kHalfMax = (T(1) << kShift) - 1;
      T a_lo = a & kHalfMax;
      T a_hi = a >> kShift;
      T b_lo = b & kHalfMax;
      T b_hi = b >> kShift;
      T x = a_lo * b_lo;
      T y = a_lo * b_hi;
      T z = b_lo * a_hi;
      T w = b_hi * a_hi;
      T lo;
      T overflow1 = cbu::add_overflow(x, T(y << kShift), &lo);
      T overflow2 = cbu::add_overflow(lo, T(z << kShift), &lo);
      T hi = (y >> kShift) + (z >> kShift) + w + overflow1 + overflow2;
      return {lo, hi};
    }
  }

  constexpr bool add_overflow(const DoubleUnsigned& other) noexcept {
    bool carry = cbu::add_overflow(lo, other.lo, &lo);
    bool overflow_carry = carry ? cbu::add_overflow(hi, 1, &hi) : false;
    bool overflow_hi = cbu::add_overflow(hi, other.hi, &hi);
    return overflow_carry || overflow_hi;
  }

  constexpr bool sub_overflow(const DoubleUnsigned& other) noexcept {
    bool borrow = cbu::sub_overflow(lo, other.lo, &lo);
    bool overflow_borrow = borrow ? cbu::sub_overflow(hi, 1, &hi) : false;
    bool overflow_hi = cbu::sub_overflow(hi, other.hi, &hi);
    return overflow_borrow || overflow_hi;
  }

  constexpr bool mul_overflow(const DoubleUnsigned& other) noexcept {
    DoubleUnsigned r = from_mul(lo, other.lo);

    T x;
    bool overflow1 = cbu::mul_overflow(lo, other.hi, &x);
    T y;
    bool overflow2 = cbu::mul_overflow(hi, other.lo, &y);
    bool overflow3 = cbu::add_overflow(r.hi, x, &r.hi);
    bool overflow4 = cbu::add_overflow(r.hi, y, &r.hi);
    bool overflow5 = (hi && other.hi);
    *this = r;
    return overflow1 || overflow2 || overflow3 || overflow4 || overflow5;
  }

  constexpr bool shl_overflow(unsigned bits) noexcept {
    T olo = lo;
    T ohi = hi;
    if (bits >= 16 * sizeof(T)) {
      lo = hi = 0;
      return olo > 0 || ohi > 0;
    } else if (bits >= 8 * sizeof(T)) {
      lo = 0;
      hi = olo << (bits - 8 * sizeof(T));
      return (ohi != 0) || (hi >> (bits - 8 * sizeof(T))) != olo;
    } else if (bits > 0) {
      hi = (ohi << bits) | (olo >> (8 * sizeof(T) - bits));
      lo <<= bits;
      return (T(ohi << bits) >> bits) != ohi;
    } else {
      return false;
    }
  }

  constexpr void shr(unsigned bits) noexcept {
    if (bits >= 16 * sizeof(T)) {
      lo = hi = 0;
    } else if (bits >= 8 * sizeof(T)) {
      lo = hi >> (bits - 8 * sizeof(T));
      hi = 0;
    } else if (bits > 0) {
      lo = (lo >> bits) | (hi << (8 * sizeof(T) - bits));
      hi >>= bits;
    }
  }

  constexpr DoubleUnsigned& operator+=(const DoubleUnsigned& other) noexcept {
    add_overflow(other);
    return *this;
  }

  constexpr DoubleUnsigned& operator-=(const DoubleUnsigned& other) noexcept {
    sub_overflow(other);
    return *this;
  }

  constexpr DoubleUnsigned& operator*=(const DoubleUnsigned& other) noexcept {
    mul_overflow(other);
    return *this;
  }

  constexpr DoubleUnsigned& operator<<=(unsigned bits) noexcept {
    shl_overflow(bits);
    return *this;
  }

  constexpr DoubleUnsigned& operator>>=(unsigned bits) noexcept {
    shr(bits);
    return *this;
  }

  friend constexpr bool operator==(const DoubleUnsigned& a,
                                   const DoubleUnsigned& b) noexcept = default;

  friend constexpr std::strong_ordering operator<=>(
      const DoubleUnsigned& a, const DoubleUnsigned& b) noexcept {
    return std::pair{a.hi, a.lo} <=> std::pair{b.hi, b.lo};
  }

  friend constexpr DoubleUnsigned operator+(const DoubleUnsigned& a,
                                            const DoubleUnsigned& b) noexcept {
    return DoubleUnsigned(a) += b;
  }

  friend constexpr DoubleUnsigned operator-(const DoubleUnsigned& a,
                                            const DoubleUnsigned& b) noexcept {
    return DoubleUnsigned(a) -= b;
  }

  friend constexpr DoubleUnsigned operator*(const DoubleUnsigned& a,
                                            const DoubleUnsigned& b) noexcept {
    return DoubleUnsigned(a) *= b;
  }

  friend constexpr DoubleUnsigned operator<<(const DoubleUnsigned& a,
                                             unsigned bits) noexcept {
    return DoubleUnsigned(a) <<= bits;
  }

  friend constexpr DoubleUnsigned operator>>(const DoubleUnsigned& a,
                                             unsigned bits) noexcept {
    return DoubleUnsigned(a) >>= bits;
  }
};

} // namespace cbu
