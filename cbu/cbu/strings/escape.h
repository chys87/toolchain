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

#include <string>
#include <string_view>

#include "cbu/compat/string.h"

namespace cbu {

enum struct EscapeStyle : unsigned {
  C,
  JSON,
};

struct EscapeStringOptions {
  EscapeStyle style : 1 = EscapeStyle::C;
  bool quotes : 1 = false;

  constexpr EscapeStringOptions(EscapeStyle style = EscapeStyle::C) noexcept :
    style(style) {}

  static const EscapeStringOptions JSON;
  static const EscapeStringOptions C;

  constexpr EscapeStringOptions with_quotes(bool w = true) const noexcept {
    auto res = *this;
    res.quotes = w;
    return res;
  }

  constexpr EscapeStringOptions without_quotes() const noexcept {
    return with_quotes(false);
  }
};

inline constexpr EscapeStringOptions EscapeStringOptions::JSON =
    EscapeStringOptions(EscapeStyle::JSON);
inline constexpr EscapeStringOptions EscapeStringOptions::C =
    EscapeStringOptions(EscapeStyle::C);

char* escape_string(char* w, std::string_view src,
                    EscapeStringOptions options) noexcept;

void escape_string_append(std::string* dst, std::string_view src,
                          EscapeStringOptions options = EscapeStringOptions());

std::string escape_string(std::string_view src,
                          EscapeStringOptions options = EscapeStringOptions());

// unescape_string supports both C and JSON styles
// Unescaping terminates either by encoutering unescaped '\"' or end-of-string
enum UnescapeStringStatus : unsigned char {
  OK_QUOTE,
  OK_EOS,
  INVALID_ESCAPE,  // Invalid escape sequence
  CODE_POINT_OUT_OF_RANGE,  // Unsupported Unicode code points
  HEAD_SURROGATE_WITHOUT_TAIL,  // Head surrogate without tail
  TAIL_SURROGATE_WITHOUT_HEAD,  // Tail surrogate without head
};

struct UnescapeStringResult {
  UnescapeStringStatus status;
  char* dst_ptr;
  const char* src_ptr;
};

UnescapeStringResult unescape_string(char* dst, const char* src,
                                     const char* end) noexcept;

inline UnescapeStringResult unescape_string(char* dst,
                                            std::string_view src) noexcept {
  return unescape_string(dst, src.data(), src.data() + src.size());
}

} // namespace cbu
