/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2025, chys <admin@CHYS.INFO>
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

#include <string_view>
#include <utility>

#include "cbu/common/concepts.h"
#include "cbu/strings/str_builder.h"
#include "cbu/strings/zstring_view.h"

// str_array is a simple string array, with fixed length determined at runtime

namespace cbu {

template <bool Z>
class base_str_array {
 public:
  constexpr base_str_array() noexcept : p_(nullptr), l_(0) {}
  constexpr base_str_array(std::size_t l)
      : p_(static_cast<char*>(::operator new[](l + Z))), l_(l) {}
  constexpr base_str_array(const char* p, std::size_t l) : base_str_array(l) {
    __builtin_memcpy(p_, p, l);
    if constexpr (Z) p_[l] = '\0';
  }
  constexpr base_str_array(base_str_array&& other) noexcept
      : p_(std::exchange(other.p_, nullptr)), l_(std::exchange(other.l_, 0)) {}

  explicit constexpr base_str_array(const cbu::String_view_compat auto& other)
      : base_str_array(other.data(), other.size()) {}

  template <typename Builder>
    requires requires(const Builder& builder, char* p) {
      { builder.write(p) } noexcept -> std::same_as<char*>;
      requires sb::HasSize<Builder>;
      requires !sb::HasMaxSize<Builder>;
    }
  explicit base_str_array(const Builder& builder)
      : base_str_array(builder.size()) {
    char* w = builder.write(p_);
    if constexpr (Z) {
      *w = '\0';
    } else {
      static_cast<void>(w);
    }
  }

  template <typename Builder>
    requires requires(const Builder& builder, char* p) {
      { builder(p) } noexcept -> std::same_as<char*>;
      requires sb::HasSize<Builder>;
      requires !sb::HasMaxSize<Builder>;
    }
  explicit base_str_array(const Builder& builder)
      : base_str_array(builder.size()) {
    char* w = builder(p_);
    if constexpr (Z) {
      *w = '\0';
    } else {
      static_cast<void>(w);
    }
  }

  constexpr ~base_str_array() {
    if consteval {
      ::operator delete[](p_, l_ + Z);
    } else {
      if (!__builtin_constant_p(p_ != nullptr) || p_ != nullptr)
        ::operator delete[](p_, l_ + Z);
    }
  }

  using iterator = char*;
  using const_iterator = const char*;
  using size_type = std::size_t;
  using pointer = char*;
  using reference = char&;

  constexpr char* c_str() noexcept
    requires Z
  {
    return p_;
  }
  constexpr const char* c_str() const noexcept
    requires Z
  {
    return p_;
  }
  constexpr char* data() noexcept { return p_; }
  constexpr const char* data() const noexcept { return p_; }
  constexpr std::size_t size() const noexcept { return l_; }
  constexpr std::size_t length() const noexcept { return l_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return l_ == 0; }

  constexpr iterator begin() noexcept { return p_; }
  constexpr const_iterator begin() const noexcept { return p_; }
  constexpr const_iterator cbegin() const noexcept { return p_; }
  constexpr iterator end() noexcept { return p_ + l_; }
  constexpr const_iterator end() const noexcept { return p_ + l_; }
  constexpr const_iterator cend() const noexcept { return p_ + l_; }

  constexpr operator std::string_view() const noexcept { return {p_, l_}; }
  constexpr operator cbu::zstring_view() const noexcept
    requires Z
  {
    return {p_, l_};
  }

  template <typename T>
  constexpr T as() const noexcept {
    return T{p_, l_};
  }

 private:
  char* p_;
  std::size_t l_;
};

using str_array = base_str_array<false>;
using str_zarray = base_str_array<true>;

}  // namespace cbu
