/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

#if __has_include(<span>)

# include <span>
namespace cbu {
namespace compat {
using std::span;
} // namespace compat
} // namespace cbu

#else // !__has_include(<span>)

#include <cstddef>
#include "cbu/common/concepts.h"

namespace cbu {
namespace compat {

template <typename C, typename T>
concept Span_compatible = requires(C&& c) {
  { std::data(std::forward<C>(c)) } -> convertible_to<T*>;
  { std::size(std::forward<C>(c)) } -> convertible_to<std::size_t>;
};

// An incomplete implementation of span
template <typename T>
class span {
 public:
  constexpr span() noexcept : p_(nullptr), n_(0) {}
  constexpr span(T* p, std::size_t n) noexcept : p_(p), n_(n) {}
  constexpr span(T* a, T* b) noexcept : p_(a), n_(b - a) {}
  template <typename C>
    requires Span_compatible<C, T>
  constexpr span(C&& c) noexcept : p_(std::data(std::forward<C>(c))),
                                   n_(std::size(std::forward<C>(c))) {}

  constexpr T* data() const noexcept { return p_; }
  constexpr std::size_t size() const noexcept { return n_; }

  constexpr T* begin() const noexcept { return p_; }
  constexpr T* end() const noexcept { return (p_ + n_); }

 private:
  T* p_;
  std::size_t n_;
};

} // namespace compat
} // namespace cbu

#endif // __has_include(<span>)
