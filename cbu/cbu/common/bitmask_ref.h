/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021, chys <admin@CHYS.INFO>
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

#include <cstddef>
#include <span>

#include "cbu/common/byteorder.h"

namespace cbu {

template <typename T>
class BitRef {
 public:
  static inline constexpr unsigned kBitsPerUnit = 8 * sizeof(T);

 public:
  template <typename Size = std::size_t>
  constexpr BitRef(T* p, Size bit)
      : r_(p + bit / kBitsPerUnit), bit_(bit % kBitsPerUnit) {}
  constexpr BitRef& operator=(bool v) noexcept {
    r_ = (r_ & ~(T(1) << bit_)) | (T(v ? 1 : 0) << bit_);
    return *this;
  }
  constexpr operator bool() const noexcept { return r_ & (T(1) << bit_); }
  constexpr bool operator~() const noexcept { return !bool(*this); }
  constexpr BitRef& flip() noexcept {
    r_ = r_ ^ (T(1) << bit_);
    return *this;
  }

 private:
  // We guarantee the data is stored in little endian, so that the same memory
  // block can be accessed in different types
  FixByteOrderRef<T, std::endian::little> r_;
  unsigned bit_;
};

template <typename T, typename Size = unsigned int>
class BitMaskRef {
 public:
  static inline constexpr unsigned kBitsPerUnit = 8 * sizeof(T);
  using reference = BitRef<T>;
  static_assert(std::is_unsigned_v<Size>);

 public:
  constexpr BitMaskRef(T* p, Size bits) noexcept : p_(p), bits_(bits) {}

  constexpr BitMaskRef(std::span<T> sp) noexcept
      : p_(sp.data()), bits_(sp.size() * kBitsPerUnit) {}

  constexpr bool operator[](Size i) const noexcept {
    return BitRef(p_, i);
  }

  constexpr reference operator[](Size i) noexcept {
    return BitRef(p_, i);
  }

  constexpr bool test(Size i) noexcept { return (*this)[i]; }
  constexpr void set(Size i) noexcept { (*this)[i] = true; }
  constexpr void reset(Size i) noexcept { (*this)[i] = false; }
  constexpr void flip(Size i) noexcept { (*this)[i].flip(); }

  // Range is [lo, hi), following STL convention
  constexpr void set(Size lo, Size hi) noexcept { set_range(p_, lo, hi); }
  constexpr void reset(Size lo, Size hi) noexcept { reset_range(p_, lo, hi); }

  constexpr const T* get() const noexcept { return p_; }
  constexpr T* get() noexcept { return p_; }
  constexpr Size bits() noexcept { return bits_; }
  constexpr Size units() noexcept {
    return (bits_ + kBitsPerUnit - 1) / kBitsPerUnit;
  }

 private:
  static constexpr void set_range(T* p, Size lo, Size hi) noexcept;
  static constexpr void reset_range(T* p, Size lo, Size hi) noexcept;

 private:
  T* p_;
  Size bits_;
};

template <typename T, typename Size>
BitMaskRef(T*, Size) -> BitMaskRef<T, std::make_unsigned_t<Size>>;

template <typename T, std::size_t Extent>
BitMaskRef(std::span<T, Extent>)
    -> BitMaskRef<T, std::conditional_t<(Extent == std::dynamic_extent ||
                                         unsigned(Extent) != Extent),
                                        std::size_t, unsigned int>>;

template <typename T, std::size_t N>
BitMaskRef(T (&)[N]) -> BitMaskRef<
    T, std::conditional_t<(unsigned(N) != N), std::size_t, unsigned int>>;

template <typename T, typename Size>
constexpr void BitMaskRef<T, Size>::set_range(T* p, Size lo, Size hi) noexcept {
  if (lo >= hi) return;

  if (lo / kBitsPerUnit == (hi - 1) / kBitsPerUnit) {
    p[lo / kBitsPerUnit] |= ((T(2) << (hi - 1 - lo)) - 1)
                            << (lo % kBitsPerUnit);
    return;
  }

  p[lo / kBitsPerUnit] |= T(-1) << (lo % kBitsPerUnit);
  for (Size j = lo / kBitsPerUnit + 1; j != (hi - 1) / kBitsPerUnit; ++j)
    p[j] = T(-1);
  p[(hi - 1) / kBitsPerUnit] |= (T(2) << ((hi - 1) % kBitsPerUnit)) - 1;
}

template <typename T, typename Size>
constexpr void BitMaskRef<T, Size>::reset_range(T* p, Size lo,
                                                Size hi) noexcept {
  if (lo >= hi) return;

  if (lo / kBitsPerUnit == (hi - 1) / kBitsPerUnit) {
    p[lo / kBitsPerUnit] &= (T(-2) << ((hi - 1) % kBitsPerUnit)) |
                            ((T(1) << (lo % kBitsPerUnit)) - 1);
    return;
  }

  p[lo / kBitsPerUnit] &= (T(1) << (lo % kBitsPerUnit)) - 1;
  for (Size j = lo / kBitsPerUnit + 1; j != (hi - 1) / kBitsPerUnit; ++j)
    p[j] = 0;
  p[(hi - 1) / kBitsPerUnit] &= T(-2) << ((hi - 1) % kBitsPerUnit);
}

}  // namespace cbu
