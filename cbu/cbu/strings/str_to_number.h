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

#include <concepts>
#include <optional>
#include <string_view>

#include "cbu/common/tags.h"
#include "cbu/strings/str_to_number_internal.h"  // IWYU pragma: export

namespace cbu {

// Strict conversion from string to integral types
// This is like absl::SimpleAtoi, but supports more types and has different
// return types.
//
// str_to_integer converts the whole string, and the resulting integer is
// strictly required to be in range;
//
// str_to_integer_partial converts as many characters as possible, and returns
// the value and the endptr (pointer past the last converted character).
//
// Supported tags:
//
//   DecTag: decimal (default)
//   AutoRadixTag: auto detect radix ("0" or "0o" for oct; "0x" for hex;
//                                    otherwise dec)
//   OctTag: octal
//   HexTag: hexadecimal
//   RadixTag<radix>: Other radix (between 2 and 36)
//
//   IgnoreOverflowTag: Don't bother to check for overflow
//
//   HaltScanOnOverflowTag: Don't bother to continue scanning on overflow
//                          (str_to_integer_partial mode only, affects returned
//                           endptr)
template <std::integral T, typename... Options>
  requires(str_to_number_detail::IntegerSupported<T, Options...>)
constexpr std::optional<T> str_to_integer(const char* s, const char* e) {
  using Tag = typename str_to_number_detail::IntegerOptionParser<
      OverflowThresholdTag<str_to_number_detail::OverflowThresholdByType<T>>,
      Options...>::Tag;
  using CT = str_to_number_detail::ConversionType<T>;
  auto res = str_to_number_detail::str_to_integer<CT, false, Tag>(s, e);
  // Unsigned case is completely covered by OverflowThreshold
  if constexpr (Tag::check_overflow && sizeof(T) < sizeof(CT) &&
                std::is_signed_v<T>) {
    if (res && T(*res) != CT(*res)) return std::nullopt;
  }
  return std::optional<T>(res);
}

template <std::integral T, typename... Options>
  requires(str_to_number_detail::IntegerSupported<T, Options...>)
constexpr auto str_to_integer(std::string_view s) noexcept {
  return str_to_integer<T, Options...>(s.data(), s.data() + s.size());
}

template <std::integral T, typename... Options>
  requires(str_to_number_detail::IntegerSupported<T, Options...>)
constexpr auto str_to_integer_partial(const char* s, const char* e) noexcept
    -> StrToNumberPartialResult<T> {
  using Tag = typename str_to_number_detail::IntegerOptionParser<
      OverflowThresholdTag<str_to_number_detail::OverflowThresholdByType<T>>,
      Options...>::Tag;
  using CT = str_to_number_detail::ConversionType<T>;
  auto res = str_to_number_detail::str_to_integer<CT, true, Tag>(s, e);
  // Unsigned case is completely covered by OverflowThreshold
  if constexpr (Tag::check_overflow && sizeof(T) < sizeof(CT) &&
                std::is_signed_v<T>) {
    if (res.first && T(*res.first) != CT(*res.first))
      return {std::nullopt, res.second};
  }
  return {std::optional<T>(res.value_opt), res.endptr};
}

template <std::integral T, typename... Options>
  requires(str_to_number_detail::IntegerSupported<T, Options...>)
constexpr auto str_to_integer_partial(std::string_view s) noexcept {
  return str_to_integer_partial<T, Options...>(s.data(), s.data() + s.size());
}

// Mimics absl::SimpleAtoi
template <int base = 0, std::integral T>
constexpr bool SimpleAtoi(std::string_view s, T* res) noexcept {
  auto value_opt = str_to_integer<T, RadixTag<base>>(s);
  if (value_opt) *res = *value_opt;
  return value_opt.has_value();
}

// Conversion from string to float or double
// This is like absl::SimpleAtof/absl::SimpleAtod, but supports more types and
// has different return types.
//
// str_to_fp converts the whole string;
//
// str_to_fp_partial converts as many characters as possible, and returns
// the value and the endptr (pointer past the last converted character).
//
// Supported tags:
//
//   DecTag: decimal (default)
//   AutoRadixTag: auto detect radix ("0x" for hex; otherwise dec)
template <typename T, typename... Options>
  requires(str_to_number_detail::FpSupported<T, Options...>)
constexpr std::optional<T> str_to_fp(const char* s, const char* e) noexcept {
  return str_to_number_detail::str_to_fp<
      T, false, typename str_to_number_detail::FpOptionParser<Options...>::Tag>(
      s, e);
}

template <typename T, typename... Options>
  requires(str_to_number_detail::FpSupported<T, Options...>)
constexpr auto str_to_fp(std::string_view s) noexcept {
  return str_to_fp<T, Options...>(s.data(), s.data() + s.size());
}

template <typename T, typename... Options>
  requires(str_to_number_detail::FpSupported<T, Options...>)
constexpr auto str_to_fp_partial(const char* s, const char* e) noexcept
    -> StrToNumberPartialResult<T> {
  return str_to_number_detail::str_to_fp<
      T, true, typename str_to_number_detail::FpOptionParser<Options...>::Tag>(
      s, e);
}

template <typename T, typename... Options>
  requires(str_to_number_detail::FpSupported<T, Options...>)
constexpr auto str_to_fp_partial(std::string_view s) noexcept {
  return str_to_fp_partial<T, Options...>(s.data(), s.data() + s.size());
}

// Mimics absl::SimpleAtof and absl::SimpleAtod
constexpr bool SimpleAtof(std::string_view s, float* res) noexcept {
  auto value_opt = str_to_fp<float>(s);
  if (value_opt) *res = *value_opt;
  return value_opt.has_value();
}

constexpr bool SimpleAtod(std::string_view s, double* res) noexcept {
  auto value_opt = str_to_fp<double>(s);
  if (value_opt) *res = *value_opt;
  return value_opt.has_value();
}

}  // namespace cbu
