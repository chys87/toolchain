/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>

#include "cbu/common/byteorder.h"
#include "cbu/common/concepts.h"

namespace cbu {
inline namespace cbu_low_level_buffer_fillter {

template <Integral T, std::endian Order>
struct FillByEndian {
  T value;

  constexpr FillByEndian(T v) noexcept: value(v) {}

  template <Char_type Ch>
  Ch* to_buffer(Ch* p) const noexcept {
    T swapped = bswap_for<Order>(value);
    std::memcpy(p, &swapped, sizeof(T));
    return p + sizeof(T);
  }

  template <Char_type Ch>
  constexpr Ch* constexpr_to_buffer(Ch* p) const noexcept {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      if constexpr (Order == std::endian::little) {
        *p++ = Ch((value >> (i * 8)) & 0xff);
      } else {
        *p++ = Ch((value >> ((sizeof(T) - 1 - i) * 8)) & 0xff);
      }
    }
    return p;
  }
};

template <Integral T>
using FillLittleEndian = FillByEndian<T, std::endian::little>;
template <Integral T>
using FillBigEndian = FillByEndian<T, std::endian::big>;
template <Integral T>
using FillNativeEndian = FillByEndian<T, std::endian::native>;

// Low-level buffer filler
template <Char_type Ch>
class LowLevelBufferFiller {
 public:
  constexpr LowLevelBufferFiller() noexcept : p_(nullptr) {}
  explicit constexpr LowLevelBufferFiller(Ch* p) noexcept : p_(p) {}
  // Copy is safe, but makes no sense, so we disable it.
  LowLevelBufferFiller(const LowLevelBufferFiller&) = delete;
  constexpr LowLevelBufferFiller(LowLevelBufferFiller&& o) :
    p_(std::exchange(o.p_, nullptr)) {}

  LowLevelBufferFiller& operator=(const LowLevelBufferFiller&) = delete;
  constexpr LowLevelBufferFiller& operator=(LowLevelBufferFiller&& o) noexcept {
    swap(o);
    return *this;
  }

  constexpr void swap(LowLevelBufferFiller& o) noexcept { std::swap(p_, o.p_); }

  constexpr LowLevelBufferFiller& operator<<(Ch c) noexcept {
    *p_++ = c;
    return *this;
  }

  LowLevelBufferFiller& operator<<(std::basic_string_view<Ch> v) noexcept {
    auto size = v.size();
    p_ = static_cast<Ch*>(std::memcpy(p_, v.data(), size)) + size;
    return *this;
  }

  template <typename T>
    requires requires (T&& v) {
      {std::forward<T>(v).to_buffer(std::declval<Ch*>())} -> convertible_to<Ch*>;
    }
  LowLevelBufferFiller& operator<<(T&& v) noexcept {
    p_ = std::forward<T>(v).to_buffer(p_);
    return *this;
  }

  // operator < is less intuitive, is less efficient, doesn't guarantee
  // evaluation order even under C++20, but has the benefit of being constexpr
  constexpr LowLevelBufferFiller& operator<(Ch c) noexcept {
    *p_++ = c;
    return *this;
  }

  constexpr LowLevelBufferFiller& operator<(std::basic_string_view<Ch> v)
      noexcept {
    p_ = std::copy_n(v.data(), v.size(), p_);
    return *this;
  }

  template <typename T>
    requires requires (T&& v) {
      {std::forward<T>(v).constexpr_to_buffer(std::declval<Ch*>())} ->
      convertible_to<Ch*>;
    }
  constexpr LowLevelBufferFiller& operator<(T&& v) noexcept {
    p_ = std::forward<T>(v).constexpr_to_buffer(p_);
    return *this;
  }

  constexpr Ch* pointer() const noexcept { return p_; }

 private:
  Ch* p_;
};

} // inline namespace cbu_low_level_buffer_fillter
} // namespace cbu
