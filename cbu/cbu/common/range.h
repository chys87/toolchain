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

#include <compare>
#include <iterator>
#include <type_traits>

namespace cbu {
inline namespace cbu_range {

template <typename T>
class RangeIterator : public std::iterator<
    std::random_access_iterator_tag, T,
    std::make_signed_t<T>,
    const T*,
    T /* reference is not a real reference, as in istreambuf_iterator */ > {
public:
  using difference_type = std::make_signed_t<T>;

  explicit constexpr RangeIterator(T v) noexcept : v_(v) {}
  constexpr RangeIterator(const RangeIterator &) noexcept = default;
  constexpr RangeIterator &operator = (const RangeIterator &) noexcept
    = default;

  constexpr T operator * () const noexcept {
    return v_;
  }

  constexpr RangeIterator& operator ++ () noexcept {
    ++v_;
    return *this;
  }
  constexpr RangeIterator operator ++ (int) noexcept {
    return RangeIterator(v_++);
  }
  constexpr RangeIterator& operator -- () noexcept {
    --v_;
    return *this;
  }
  constexpr RangeIterator operator -- (int) noexcept {
    return RangeIterator(v_--);
  }
  constexpr RangeIterator& operator += (difference_type x) noexcept {
    v_ += x;
    return *this;
  }
  constexpr RangeIterator& operator -= (difference_type x) noexcept {
    v_ -= x;
    return *this;
  }
  constexpr T operator[](difference_type x) const noexcept {
    return (v_ + x);
  }

  friend constexpr bool operator == (const RangeIterator& a,
                                     const RangeIterator& b) noexcept {
    return a.v_ == b.v_;
  }

  friend constexpr auto operator <=> (const RangeIterator& a,
                                      const RangeIterator& b) noexcept {
    return a.v_ <=> b.v_;
  }

private:
  T v_;
};

template <typename T>
inline constexpr RangeIterator<T> operator+ (
    const RangeIterator<T>& a,
    typename RangeIterator<T>::difference_type b) {
  return RangeIterator<T>(a) += b;
}

template <typename T>
inline constexpr RangeIterator<T> operator+ (
    typename RangeIterator<T>::difference_type b,
    const RangeIterator<T>& a) {
  return a + b;
}

template <typename T>
inline constexpr typename RangeIterator<T>::difference_type
operator- (const RangeIterator<T>& a, const RangeIterator<T>& b) {
  return *a - *b;
}

template <typename T>
class Range {
 public:
  explicit constexpr Range(T from, T to) noexcept :
    from_(from), to_(from < to ? to : from) {}
  explicit constexpr Range(T to) noexcept : from_(0), to_(to) {}

  using value_type = T;
  using iterator = RangeIterator<T>;
  using const_iterator = iterator;

  constexpr iterator begin() const noexcept { return iterator(from_); }
  constexpr iterator end() const noexcept { return iterator(to_); }
  constexpr iterator cbegin() const noexcept { return iterator(from_); }
  constexpr iterator cend() const noexcept { return iterator(to_); }

  [[nodiscard]] constexpr bool empty() const noexcept { return (from_ == to_); }
  constexpr T size() const noexcept { return (to_ - from_); }

 private:
  T from_;
  T to_;
};

template <typename T, typename U>
Range(T, U) -> Range<std::common_type_t<T, U>>;

static_assert(std::is_same_v<decltype(Range(1, 2l)), Range<long>>);
static_assert(std::is_same_v<decltype(Range(1, 2u)), Range<unsigned int>>);
static_assert(std::is_same_v<decltype(Range(7)), Range<int>>);

} // namespace cbu_range
} // namespace cbu
