/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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
#include <type_traits>

#include "cbu/compat/compilers.h"

namespace cbu {
namespace detail {

void do_munmap(void*, std::size_t) noexcept;

}  // namespace detail

template <typename T>
concept ScopedMMapCharType =
    std::is_void_v<T> || (std::is_integral_v<T> && sizeof(T) == 1) ||
    std::is_same_v<T, std::byte>;

template <ScopedMMapCharType T = void>
class CBU_TRIVIAL_ABI ScopedMMap {
 public:
  constexpr ScopedMMap() noexcept = default;
  explicit constexpr ScopedMMap(T* p, std::size_t l) noexcept : p_(p), l_(l) {}
  ScopedMMap(const ScopedMMap&) = delete;
  ScopedMMap(ScopedMMap&& other) noexcept
      : p_(std::exchange(other.p_)), l_(std::exchange(other.l_, 0)) {}
  [[gnu::always_inline]] ~ScopedMMap() noexcept {
    if (l_ > 0) detail::do_munmap(p_, l_);
  }

  ScopedMMap& operator=(const ScopedMMap&) = delete;
  ScopedMMap& operator=(ScopedMMap&& other) noexcept {
    swap(other);
    return *this;
  }

  void swap(ScopedMMap& other) noexcept {
    std::swap(p_, other.p_);
    std::swap(l_, other.l_);
  }

  constexpr T* ptr() const noexcept { return p_; }
  constexpr std::size_t size() const noexcept { return l_; }

  std::pair<T*, std::size_t> release() noexcept {
    return {std::exchange(p_, nullptr), std::exchange(l_, 0)};
  }

  void reset() noexcept {
    if (l_ > 0)
      detail::do_munmap(std::exchange(p_, nullptr), std::exchange(l_, 0));
  }
  void reset(T* p, std::size_t l) noexcept {
    if (l_ > 0) detail::do_munmap(std::exchange(p_, p), std::exchange(l_, l));
  }

  constexpr void reset_unsafe(T* p, std::size_t l) noexcept {
    p_ = p;
    l_ = l;
  }

 public:
  static constexpr inline bool bitwise_movable(ScopedMMap*) { return true; }

 private:
  T* p_ = nullptr;
  std::size_t l_ = 0;
};

}  // namespace cbu
