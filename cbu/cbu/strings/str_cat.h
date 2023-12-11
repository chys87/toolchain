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
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

// Replacement of absl::StrCat and absl::StrAppend which supports std::string_view
// in builds where absl::string_view isn't std::string_view

namespace cbu {
namespace str_cat_detail {

void Append(std::string* dst, const std::string_view* array, std::size_t cnt,
            std::size_t total_size);
std::string Cat(const std::string_view* array, std::size_t cnt,
                std::size_t total_size);

// This needs to go before SupportedAsStringView, so that the length of
// const char (&)[N] is determined by strlen, instead of just using N
template <typename T>
concept SupportedAsConstCharPtr = std::convertible_to<T, const char*>;

template <typename T>
concept SupportedAsStringView =
    !SupportedAsConstCharPtr<T> && requires(const T& v) {
      { std::data(v) } -> std::convertible_to<const char*>;
      { std::size(v) } -> std::convertible_to<std::size_t>;
    };

template <typename T>
concept SupportedAsIntegral =
    std::integral<T> && !std::is_same_v<std::remove_cvref_t<T>, bool>;

template <typename T>
concept Supported = SupportedAsConstCharPtr<T> || SupportedAsStringView<T> ||
                    SupportedAsIntegral<T>;

template <typename T>
struct TempBuffer {};

template <std::unsigned_integral T>
struct TempBuffer<T> {
  char buffer[std::numeric_limits<T>::digits10 + 1];
};

template <std::signed_integral T>
struct TempBuffer<T> {
  char buffer[std::numeric_limits<T>::digits10 + 2];
};

constexpr std::string_view Prepare(
    auto* buffer, const SupportedAsConstCharPtr auto& s) noexcept {
  return std::string_view(s, __builtin_strlen(s));
}

constexpr std::string_view Prepare(
    auto* buffer, const SupportedAsStringView auto& sv) noexcept {
  return std::string_view(std::data(sv), std::size(sv));
}

template <std::integral T>
constexpr std::string_view Prepare(TempBuffer<T>* buffer, T v) noexcept {
  using U = std::make_unsigned_t<T>;
  char* p = buffer->buffer;

  U u = v;
  if (v < 0) {
    u = -v;
    *p++ = '-';
  }

  char* w = p;
  do {
    *w++ = u % 10 + '0';
  } while (u /= 10);
  unsigned char l = w - buffer->buffer;
  while (--w > p) {
    unsigned char ow = *w;
    unsigned char op = *p;
    *p = ow;
    *w = op;
    ++p;
  }
  return {buffer->buffer, l};
}

template <std::size_t... Idx, typename Tpl>
inline void StrAppend(std::string* dst, std::index_sequence<Idx...>,
                      const Tpl& args) {
  std::tuple<str_cat_detail::TempBuffer<
      std::remove_cvref_t<std::tuple_element_t<Idx, Tpl>>>...>
      buffers;

  std::size_t total_size = 0;
  auto prepare = [&total_size](auto* buffer, const auto& v) constexpr noexcept {
    std::string_view sv = Prepare(buffer, v);
    total_size += sv.size();
    return sv;
  };

  std::string_view svs[]{
      prepare(&std::get<Idx>(buffers), std::get<Idx>(args))...};

  Append(dst, svs, std::size(svs), total_size);
}

template <std::size_t... Idx, typename Tpl>
inline std::string StrCat(std::index_sequence<Idx...>, const Tpl& args) {
  std::tuple<str_cat_detail::TempBuffer<
      std::remove_cvref_t<std::tuple_element_t<Idx, Tpl>>>...>
      buffers;

  std::size_t total_size = 0;
  auto prepare = [&total_size](auto* buffer, const auto& v) constexpr noexcept {
    std::string_view sv = Prepare(buffer, v);
    total_size += sv.size();
    return sv;
  };

  std::string_view svs[]{
      prepare(&std::get<Idx>(buffers), std::get<Idx>(args))...};

  return Cat(svs, std::size(svs), total_size);
}

}  // namespace str_cat_detail

template <str_cat_detail::Supported T>
inline constexpr void StrAppend(std::string* dst, const T& arg) {
  str_cat_detail::TempBuffer<T> buffer;
  dst->append(str_cat_detail::Prepare(&buffer, arg));
}

template <str_cat_detail::Supported... T>
  requires(sizeof...(T) > 1)
inline void StrAppend(std::string* dst, const T&... args) {
  str_cat_detail::StrAppend(dst, std::make_index_sequence<sizeof...(T)>(),
                            std::tuple<const T&...>{args...});
}

template <str_cat_detail::Supported T>
inline constexpr std::string StrCat(const T& arg) {
  str_cat_detail::TempBuffer<T> buffer;
  return std::string(str_cat_detail::Prepare(&buffer, arg));
}

template <str_cat_detail::Supported... T>
  requires(sizeof...(T) > 1)
inline std::string StrCat(const T&... args) {
  return str_cat_detail::StrCat(std::make_index_sequence<sizeof...(T)>(),
                                std::tuple<const T&...>{args...});
}

}  // namespace cbu
