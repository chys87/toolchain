/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2024, chys <admin@CHYS.INFO>
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
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "cbu/strings/fixed_string.h"

namespace cbu {
namespace string_collection_detail {

template <std::size_t MaxSize>
using len_t = std::conditional_t<
    MaxSize <= 255, std::uint8_t,
    std::conditional_t<
        MaxSize <= 65535, std::uint16_t,
        std::conditional_t<MaxSize <= 0xffffffff, std::uint32_t, std::size_t>>>;

template <fixed_string S>
constexpr auto fixed_string_instance{S};

template <std::size_t N, std::size_t MaxBytes>
struct PreprocessResult {
  char buffer[MaxBytes];
  len_t<MaxBytes> used_bytes;
  std::string_view refs[N];
  len_t<MaxBytes> offsets[N];
};

template <fixed_string... Strings>
constexpr auto preprocess() noexcept {
  constexpr std::size_t n = sizeof...(Strings);
  PreprocessResult<n, (... + Strings.size())> res{};

  auto& refs = res.refs;

  std::string_view* w = refs;
  ((*w++ = std::string_view(fixed_string_instance<Strings>)), ...);
  std::ranges::sort(
      refs, [](std::string_view a, std::string_view b) constexpr noexcept {
        return (a.size() > b.size()) || (a.size() == b.size() && a < b);
      });

  std::size_t offset = 0;
  for (std::size_t i = 0; i < n; ++i) {
    bool found = false;
    for (std::size_t j = 0; j < i; ++j) {
      if (auto pos = refs[j].find(refs[i]); pos != std::string_view::npos) {
        found = true;
        res.offsets[i] = res.offsets[j] + pos;
        break;
      }
    }
    if (!found) {
      res.offsets[i] = offset;
      std::copy_n(refs[i].data(), refs[i].size(), res.buffer + offset);
      offset += refs[i].size();
    }
  }

  res.used_bytes = offset;
  return res;
}

}  // namespace string_collection_detail

template <fixed_string... Strings>
  requires(sizeof...(Strings) > 0)
struct string_collection {
  static constexpr auto preprocessed =
      string_collection_detail::preprocess<Strings...>();

  using offset_type =
      string_collection_detail::len_t<std::ranges::max(preprocessed.offsets)>;
  using len_type =
      string_collection_detail::len_t<std::ranges::max({Strings.size()...})>;

  constexpr string_collection() noexcept {
    std::copy_n(preprocessed.buffer, preprocessed.used_bytes, s_);
  }

  static constexpr const string_collection& instance() noexcept;

  struct ref_t {
    offset_type offset;
    len_type len;

    constexpr std::string_view view() const noexcept { return {data(), len}; }
    constexpr operator std::string_view() const noexcept { return view(); }

    constexpr const char* data() const noexcept {
      return instance().s_ + offset;
    }
    constexpr len_type size() const noexcept { return len; }
  };

  template <fixed_string Needle>
    requires(... || (std::string_view(Needle) == std::string_view(Strings)))
  static consteval ref_t find() noexcept {
    for (std::size_t i = 0; i < sizeof...(Strings); ++i) {
      if (preprocessed.refs[i] == std::string_view(Needle)) {
        return {offset_type(preprocessed.offsets[i]), len_type(Needle.size())};
      }
    }
    return {};
  }

  template <fixed_string Needle>
    requires(... || (std::string_view(Needle) == std::string_view(Strings)))
  static constexpr ref_t ref = find<Needle>();

  char s_[preprocessed.used_bytes];
};

template <fixed_string... Strings>
  requires(sizeof...(Strings) > 0)
constexpr const string_collection<Strings...>&
string_collection<Strings...>::instance() noexcept {
  static constexpr string_collection obj;
  return obj;
}

}  // namespace cbu
