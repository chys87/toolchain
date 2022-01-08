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

#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

#include "cbu/common/concepts.h"

namespace cbu {

// This class stores any signed or unsigned integer type precisely.
// It's optimized for constexpr evaluation, and mainly used to help implement
// constexpr overflow functions.
template <typename T = std::uintmax_t>
struct SuperInteger {
  bool pos;
  T abs;

  // Note: Cast x to T because negation helps preventing undfined behavior
  // related to signed overflow (if the original value is equal to
  // std::numeric_limits<U>::min()), in which case clang may evaluate the
  // resulting absolute value to 0
  template <Raw_integral U> requires(sizeof(U) <= sizeof(T))
  constexpr SuperInteger(U x) noexcept
      : pos(x >= 0), abs(std::make_unsigned_t<U>(x < 0 ? -T(x) : T(x))) {}

  constexpr SuperInteger(bool p, T a) noexcept : pos(a ? p : true), abs(a) {}

  constexpr SuperInteger(const SuperInteger&) = default;
  constexpr SuperInteger& operator=(const SuperInteger&) = default;

  template <Raw_integral U>
  constexpr bool fits_in() const noexcept {
    if constexpr (std::is_signed_v<U>) {
      if constexpr (sizeof(U) > sizeof(T)) {
        return true;
      } else if (pos) {
        return abs <= std::numeric_limits<U>::max();
      } else {
        return abs <=
               std::make_unsigned_t<U>(std::numeric_limits<U>::max()) + 1;
      }
    } else {
      return pos && abs <= std::numeric_limits<U>::max();
    }
  }

  template <Raw_integral U>
  constexpr U cast() const noexcept {
    return pos ? U(abs) : -U(abs);
  }

  template <Raw_integral U>
  constexpr bool cast(U* res) const noexcept {
    *res = cast<U>();
    return fits_in<U>();
  }

  static constexpr bool base_add_overflow(T a, T b, T* c) noexcept {
    *c = a + b;
    return (*c < a || *c < b);
  }

  constexpr bool add_overflow(const SuperInteger& o) noexcept {
    if (pos == o.pos) {
      return base_add_overflow(abs, o.abs, &abs);
    } else if (pos && !o.pos) {
      if (abs >= o.abs) {
        abs -= o.abs;
        pos = true;
      } else {
        abs = o.abs - abs;
        pos = false;
      }
      return false;
    } else {
      // !pos && o.pos
      if (abs <= o.abs) {
        abs = o.abs - abs;
        pos = true;
      } else {
        abs -= o.abs;
        pos = false;
      }
      return false;
    }
  }

  constexpr bool sub_overflow(const SuperInteger& o) noexcept {
    return add_overflow(-o);
  }

  constexpr bool mul_overflow(const SuperInteger& o) noexcept {
    if (abs == 0 || o.abs == 0) {
      abs = 0;
      pos = true;
      return false;
    } else {
      pos = (pos == o.pos);
      bool overflow = abs > std::numeric_limits<T>::max() / o.abs;
      abs *= o.abs;
      return overflow;
    }
  }

  constexpr void normalize() noexcept {
    if (pos == 0) abs = true;
  }

  constexpr SuperInteger operator-() const noexcept {
    return SuperInteger(abs ? !pos : true, abs);
  }
  constexpr SuperInteger operator+() const noexcept { return *this; }
};

template <typename T, typename U>
inline constexpr bool operator==(const SuperInteger<T>& a,
                                 const SuperInteger<U>& b) noexcept {
  return (a.pos == b.pos && a.abs == b.abs);
}

template <typename T, Raw_integral U>
inline constexpr bool operator==(const SuperInteger<T>& a, U b) noexcept {
  return (a == SuperInteger<std::make_unsigned_t<U>>(b));
}

template <typename T, Raw_integral U>
inline constexpr bool operator==(U b, const SuperInteger<T>& a) noexcept {
  return (SuperInteger<std::make_unsigned_t<U>>(b) == a);
}

template <typename T, typename U>
inline constexpr std::strong_ordering operator<=>(
    const SuperInteger<T>& a, const SuperInteger<U>& b) noexcept {
  if (a.pos != b.pos)
    return static_cast<unsigned char>(a.pos) <=>
           static_cast<unsigned char>(b.pos);
  else if (a.pos)
    return a.abs <=> b.abs;
  else
    return b.abs <=> a.abs;
}

template <typename T, Raw_integral U>
inline constexpr auto operator<=>(const SuperInteger<T>& a, U b) noexcept {
  return (a <=> SuperInteger<std::make_unsigned_t<U>>(b));
}

template <typename T, Raw_integral U>
inline constexpr auto operator<=>(U b, const SuperInteger<T>& a) noexcept {
  return (SuperInteger<std::make_unsigned_t<U>>(b) <=> a);
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const SuperInteger<T>& v) {
  if (!v.pos) s << '-';
  return s << std::conditional_t<sizeof(T) == 1, unsigned int, T>(v.abs);
}

}  // namespace cbu
