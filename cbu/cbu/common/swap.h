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

#include <utility>

#include "cbu/common/type_traits.h"

// Like <boost/core/swap.hpp>, but we also support swapping by member functions
// See comments in <boost/core/swap.hpp>

// See comments in boost why we use cbu_swap_detail instead of cbu::swap_detail
namespace cbu_swap_detail {

// Fall back to std::swap only if argument dependent lookup fails
using namespace std;

template <typename T, typename U>
concept Member_swap = requires (T &a, U &b) { a.swap(b); };

// When a.swap(b) is available, use it; otherwise, fall back to ADL swap() or
// std::swap
template <typename T, typename U>
requires Member_swap<T, U>
inline constexpr void swap_impl(T &a, U &b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}

template <typename T, typename U>
requires (!Member_swap<T, U>)
inline constexpr void swap_impl(T &a, U &b) noexcept(noexcept(swap(a, b))) {
  if !consteval {
    if constexpr (std::is_same_v<T, U> && cbu::bitwise_movable_v<T>) {
      char tmp[sizeof(T)];
      __builtin_memcpy(tmp, std::addressof(a), sizeof(T));
      __builtin_memcpy(std::addressof(a), std::addressof(b), sizeof(T));
      __builtin_memcpy(std::addressof(b), tmp, sizeof(T));
      return;
    }
  }
  swap(a, b);
}

template <typename T, typename U, std::size_t N>
inline constexpr void swap_impl(T (&a)[N], U (&b)[N])
    noexcept(noexcept(swap_impl(a[0], b[0]))) {
  for (std::size_t i = 0; i < N; ++i)
    swap_impl(a[i], b[i]);
}

} // namespace cbu_swap_detail

namespace cbu {

template <typename T, typename U>
inline constexpr void swap(T &a, U &b)
    noexcept(noexcept(cbu_swap_detail::swap_impl(a, b))) {
  cbu_swap_detail::swap_impl(a, b);
}

} // namespace cbu
