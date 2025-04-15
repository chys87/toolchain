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

#include <concepts>
#include <cstddef>
#include <string_view>

#include "cbu/common/shared_instance.h"
#include "cbu/strings/fixed_string.h"

namespace cbu {

struct length_prefixed_string {
  const char* p;

  constexpr length_prefixed_string() noexcept : p(nullptr) {}
  constexpr length_prefixed_string(decltype(nullptr)) noexcept : p(nullptr) {}
  constexpr length_prefixed_string(
      const length_prefixed_string& other) noexcept = default;

  static constexpr length_prefixed_string from(const char* p) noexcept {
    length_prefixed_string r;
    r.p = p;
    return r;
  }

  // Prevent unintended initialization from string literal
  constexpr length_prefixed_string(const char*) = delete;

  constexpr length_prefixed_string& operator=(
      const length_prefixed_string&) noexcept = default;

  constexpr explicit operator bool() const noexcept { return p; }
  constexpr operator std::string_view() const noexcept { return view(); }
  constexpr std::string_view view() const noexcept {
    return {p + 1, static_cast<unsigned char>(*p)};
  }
  constexpr const char* raw_data() const noexcept { return p; }
  constexpr const char* data() const noexcept { return p + 1; }
  constexpr unsigned char size() const noexcept {
    return static_cast<unsigned char>(*p);
  }
  constexpr unsigned char length() const noexcept {
    return static_cast<unsigned char>(*p);
  }

  // The caller should know what they are doing by knowing that there is
  // at least one following string
  constexpr length_prefixed_string next() const noexcept {
    return from(p + static_cast<unsigned char>(*p) + 1);
  }

  struct StrBuilder;
  constexpr auto str_builder() const noexcept;
};

struct length_prefixed_string::StrBuilder {
  length_prefixed_string s;

  static constexpr unsigned char static_max_size() noexcept { return 255; }
  constexpr unsigned char size() const noexcept { return s.size(); }
  constexpr unsigned char min_size() const noexcept { return s.size(); }
  constexpr char* write(char* w) const noexcept {
    std::size_t l = s.size();
    if consteval {
      std::copy_n(s.data(), l, w);
    } else {
      std::memcpy(w, s.data(), l);
    }
    return w + l;
  }
};

constexpr auto length_prefixed_string::str_builder() const noexcept {
  return StrBuilder{*this};
}

template <std::size_t... Ns, bool... Zs>
  requires((sizeof...(Ns) > 0) && ... && (Ns <= 255))
constexpr cbu::fixed_nzstring<((Ns + 1) + ...)> add_length_prefix(
    const cbu::basic_fixed_string<Ns, Zs>&... s) noexcept {
  return ((fixed_nzstring<1>{{char(static_cast<unsigned char>(Ns))}} + s) +
          ...);
}

template <fixed_nzstring... ss>
  requires((sizeof...(ss) > 0) && ... && (ss.size() <= 255))
inline constexpr length_prefixed_string lp =
    length_prefixed_string::from(as_shared<add_length_prefix(ss...)>.data());

// for libfmt 10+
constexpr inline std::string_view format_as(
    const length_prefixed_string& s) noexcept {
  return s.view();
}

}  // namespace cbu
