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

#include <cstddef>
#include <functional>
#include <iterator>

namespace cbu {
inline namespace cbu_utility {


// POD equivalent of std::pair
template <typename U, typename V>
struct pair {
  [[no_unique_address]] U first;
  [[no_unique_address]] V second;

  using bitwise_movable_chain = pair(U, V);
};

template <typename U, typename V>
pair(U, V) -> pair<U, V>;

// For use with range-based for
template <typename IT>
class IteratorRange {
 public:
  typedef IT iterator;
  typedef std::reverse_iterator<IT> reverse_iterator;
  using difference_type = typename std::iterator_traits<IT>::difference_type;
  using size_type = typename std::make_unsigned<difference_type>::type;

  constexpr IteratorRange(IT lo, IT hi) noexcept : lo_(lo), hi_(hi) {}
  constexpr IteratorRange(IT lo, size_type n) noexcept
    : lo_(lo), hi_(std::next(lo, n)) {}

  constexpr IT begin() const noexcept { return lo_; }
  constexpr IT cbegin() const noexcept { return lo_; }
  constexpr IT end() const noexcept { return hi_; }
  constexpr IT cend() const noexcept { return hi_; }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(hi_);
  }
  constexpr reverse_iterator crbegin() const noexcept {
    return reverse_iterator(hi_);
  }
  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(lo_);
  }
  constexpr reverse_iterator crend() const noexcept {
    return reverse_iterator(lo_);
  }

  constexpr size_type size() const noexcept { return std::distance(lo_, hi_); }
  [[nodiscard]] constexpr bool empty() const noexcept { return lo_ == hi_; }

 private:
  IT lo_, hi_;
};

template <typename IT>
inline constexpr IteratorRange<std::reverse_iterator<IT>> reversed(
    IT begin, IT end) noexcept {
  return {std::reverse_iterator<IT>(end), std::reverse_iterator<IT>(begin)};
}

template <typename Container>
requires requires(Container &cont) {
  std::rbegin(cont);
  std::rend(cont);
}
inline constexpr auto reversed(Container &container) noexcept {
  return IteratorRange{std::rbegin(container), std::rend(container)};
}


// Reverse a comparator
template <typename T>
class ReversedComparator {
public:
  ReversedComparator() noexcept(noexcept(T())) : comp_() {}
  explicit ReversedComparator(T comp) noexcept(noexcept(T(std::move(comp)))) :
    comp_(std::move(comp)) {
  }

  template <typename U, typename V>
  auto operator () (U &&u, V &&v) const
      noexcept(noexcept(std::declval<const T&>()(std::forward<V>(v),
                                                 std::forward<U>(u)))) {
    return comp_(std::forward<V>(v), std::forward<U>(u));
  }

private:
  T comp_;
};

} // inline namespace cbu_utility
} // namespace cbu
