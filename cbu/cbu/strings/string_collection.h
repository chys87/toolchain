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
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <ranges>
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

template <std::unsigned_integral T>
struct ReduceParams {
  using type = T;

  // real_value = storage_value * a + b;
  T a;
  T b;
  T smax;  // storage max

  constexpr T rmax() const noexcept { return a * smax + b; }

  constexpr T s2r(T v) const noexcept { return a * v + b; }
  constexpr T r2s(T v) const noexcept { return (v - b) / a; }
};

template <std::unsigned_integral T>
constexpr ReduceParams<T> reduce_integers(const T* values, std::size_t n) noexcept {
  T min = std::ranges::min(std::ranges::views::counted(values, n));
  T max = std::ranges::max(std::ranges::views::counted(values, n));
  if (max == min) {
    return {0, min, 0};
  }
  T g = 0;
  for (std::size_t i = 0; i < n; ++i) {
    T r = values[i] - min;
    g = std::gcd(g, r);
    if (g == 1) return {1, min, T(max - min)};
  }
  return {g, min, T((max - min) / g)};
}

template <std::size_t N, std::size_t MaxBytes>
struct PreprocessResult {
  char buffer[MaxBytes];
  len_t<MaxBytes> used_bytes;
  std::string_view refs[N];
  len_t<MaxBytes> offsets[N];
  ReduceParams<len_t<MaxBytes>> offsets_params;
  ReduceParams<len_t<MaxBytes>> lens_params;
};

template <fixed_string... Strings>
constexpr auto preprocess() noexcept {
  constexpr std::size_t n = sizeof...(Strings);
  constexpr std::size_t MaxBytes = (... + Strings.size());
  PreprocessResult<n, MaxBytes> res{};

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
  res.offsets_params = reduce_integers(res.offsets, n);

  len_t<MaxBytes> lens[n]{Strings.size()...};
  res.lens_params = reduce_integers(lens, n);
  return res;
}

template <ReduceParams params>
struct Storage {
  len_t<params.rmax()> value;

  Storage() noexcept = default;
  constexpr Storage(len_t<params.rmax()> v) noexcept { set(v); }
  constexpr void set(len_t<params.rmax()> v) noexcept { value = v; }
  constexpr auto get() const noexcept { return value; }
};

template <ReduceParams params>
  requires(params.smax == 0)
struct Storage<params> {
  constexpr Storage(std::size_t) noexcept {}
  static constexpr void set(std::size_t) noexcept {}
  static constexpr typename decltype(params)::type get() noexcept {
    return params.b;
  }
};

template <ReduceParams params>
  requires(params.smax > 0 &&
           sizeof(len_t<params.smax>) < sizeof(len_t<params.rmax()>))
struct Storage<params> {
  len_t<params.smax> svalue = 0;

  constexpr Storage() noexcept = default;
  constexpr Storage(len_t<params.rmax()> v) noexcept { set(v); }
  constexpr void set(len_t<params.rmax()> v) noexcept {
    svalue = params.r2s(v);
  }
  constexpr len_t<params.rmax()> get() const noexcept {
    return params.s2r(svalue);
  }
};

constexpr unsigned required_bits(std::uint64_t value) noexcept {
  return 64 - std::countl_zero(value);
}

}  // namespace string_collection_detail

template <fixed_string... Strings>
  requires(sizeof...(Strings) > 0)
struct string_collection {
  static constexpr auto preprocessed =
      string_collection_detail::preprocess<Strings...>();
  static constexpr auto offsets_params = preprocessed.offsets_params;
  static constexpr auto lens_params = preprocessed.lens_params;
  static constexpr std::size_t kMaxLen = std::ranges::max({Strings.size()...});

  using offset_type =
      string_collection_detail::len_t<std::ranges::max(preprocessed.offsets)>;
  using len_type = string_collection_detail::len_t<kMaxLen>;

  constexpr string_collection() noexcept {
    std::copy_n(preprocessed.buffer, preprocessed.used_bytes, s_);
  }

  static constexpr const string_collection& instance() noexcept;

  struct ref_regular_t {
    static constexpr std::size_t kMaxLen = string_collection::kMaxLen;

    [[no_unique_address]] string_collection_detail::Storage<offsets_params>
        offset;
    [[no_unique_address]] string_collection_detail::Storage<lens_params> len;

    constexpr std::string_view view() const noexcept { return {data(), len.get()}; }
    constexpr operator std::string_view() const noexcept { return view(); }
    constexpr const char* data() const noexcept {
      return instance().s_ + offset.get();
    }
    constexpr len_type size() const noexcept { return len.get(); }
  };

  struct ref_packed_t {
    static constexpr std::size_t kMaxLen = string_collection::kMaxLen;
    static constexpr unsigned kOffsetBits =
        string_collection_detail::required_bits(offsets_params.smax);
    static constexpr unsigned kLenBits =
        string_collection_detail::required_bits(lens_params.smax);
    using type =
        string_collection_detail::len_t<(1zu << (kOffsetBits + kLenBits)) - 1>;
    type svalue = 0;

    constexpr ref_packed_t() noexcept = default;
    constexpr ref_packed_t(offset_type offset, offset_type len) noexcept
        : svalue(offsets_params.r2s(offset) |
                 (type(lens_params.r2s(len)) << kOffsetBits)) {}
    constexpr std::string_view view() const noexcept { return {data(), size()}; }
    constexpr operator std::string_view() const noexcept { return view(); }
    constexpr const char* data() const noexcept {
      return instance().s_ +
             offsets_params.s2r(svalue & ((offset_type(1) << kOffsetBits) - 1));
    }
    constexpr len_type size() const noexcept {
      return lens_params.s2r(svalue >> kOffsetBits);
    }
  };

  using ref_t =
      std::conditional_t<(sizeof(ref_packed_t) < sizeof(ref_regular_t)),
                         ref_packed_t, ref_regular_t>;

  template <fixed_string Needle>
    requires(... || (std::string_view(Needle) == std::string_view(Strings)))
  static consteval ref_t find() noexcept {
    for (std::size_t i = 0; i < sizeof...(Strings); ++i) {
      if (preprocessed.refs[i] == std::string_view(Needle)) {
        return {offset_type(preprocessed.offsets[i]), len_type(Needle.size())};
      }
    }
    return {0, 0};
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
