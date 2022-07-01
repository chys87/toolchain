/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2022, chys <admin@CHYS.INFO>
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

namespace cbu {

// The intended use of fixed_string is as template argument, see P0732R2
// (https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf), part
// of C++20.
template <std::size_t N>
struct fixed_string {
  constexpr fixed_string() noexcept : s_{} {}
  constexpr fixed_string(const char (&s)[N + 1]) noexcept {
    for (std::size_t i = 0; i < N + 1; ++i) s_[i] = s[i];
  }

  constexpr char* data() noexcept { return s_; }
  constexpr const char* data() const noexcept { return s_; }
  constexpr const char* c_str() const noexcept { return s_; }
  static constexpr std::size_t size() noexcept { return N; }
  static constexpr std::size_t length() noexcept { return N; }

  constexpr char& operator[](std::size_t n) noexcept { return s_[n]; }
  constexpr const char& operator[](std::size_t n) const noexcept {
    return s_[n];
  }

  using iterator = char*;
  using const_iterator = const char*;
  constexpr iterator begin() noexcept { return s_; }
  constexpr iterator end() noexcept { return s_; }
  constexpr const_iterator begin() const noexcept { return s_; }
  constexpr const_iterator end() const noexcept { return s_; }
  constexpr const_iterator cbegin() const noexcept { return s_; }
  constexpr const_iterator cend() const noexcept { return s_; }

  // Has to be public to be used as template argument
  char s_[N + 1];
};

template <std::size_t N>
  requires(N > 0)
fixed_string(const char (&)[N])
->fixed_string<N - 1>;

}  // namespace cbu
