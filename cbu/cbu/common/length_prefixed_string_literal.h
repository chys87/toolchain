/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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
#include <cstdint>
#include <string_view>

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

namespace cbu {

template <typename C, typename Len>
struct LengthPrefixedStringLiteral {
  Len len;
  C buffer[0];

  using string_view = std::basic_string_view<C>;

  constexpr operator string_view() const noexcept { return view(); }
  constexpr string_view view() const noexcept { return {buffer, len}; }
};

template <typename C, typename Len, C... chars>
struct LengthPrefixedStringLiteralInstance
    : LengthPrefixedStringLiteral<C, Len> {
  static_assert(Len(sizeof...(chars)) == sizeof...(chars), "Len is too small");

  C real_buffer[sizeof...(chars)]{chars...};

  using string_view = typename LengthPrefixedStringLiteral<C, Len>::string_view;

  constexpr LengthPrefixedStringLiteralInstance() noexcept
      : LengthPrefixedStringLiteral<C, Len>{Len(sizeof...(chars))} {}

  constexpr operator string_view() const noexcept { return view(); }
  constexpr string_view view() const noexcept {
    return {real_buffer, sizeof...(chars)};
  }
};

template <typename C, typename Len, C... chars>
inline constexpr LengthPrefixedStringLiteralInstance<C, Len, chars...>
    kLengthPrefixedStringLiteralInstance;

template <typename C, C... chars>
constexpr const auto& operator""_lpsl() noexcept {
  using T = LengthPrefixedStringLiteralInstance<C, std::uint8_t, chars...>;
  using B = LengthPrefixedStringLiteral<C, std::uint8_t>;
  static_assert(sizeof(B) == 1);
  static_assert(sizeof(T) == sizeof(B) + sizeof...(chars) * sizeof(C));
  return kLengthPrefixedStringLiteralInstance<C, std::uint8_t, chars...>;
}

template <typename C, C... chars>
constexpr const auto& operator""_lpsl16() noexcept {
  using T = LengthPrefixedStringLiteralInstance<C, std::uint16_t, chars...>;
  using B = LengthPrefixedStringLiteral<C, std::uint16_t>;
  static_assert(sizeof(B) == 2);
  static_assert(sizeof(T) == sizeof(B) + sizeof...(chars) * sizeof(C));
  return kLengthPrefixedStringLiteralInstance<C, std::uint16_t, chars...>;
}

}  // namespace cbu

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
