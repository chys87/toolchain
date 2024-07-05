/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include "cbu/tweak/tweak.h"

namespace cbu {

enum struct EscapeStyle : unsigned {
  C,
  JSON,         // '/' is not escaped
  JSON_STRICT,  // '/' is escaped
};

namespace escape_detail {

template <EscapeStyle style>
struct EscapeImpl {
  static char* raw(char* w, const char* s, std::size_t l) noexcept;

  static void append(std::string* dst, const char* s, std::size_t l) CBU_MEMORY_NOEXCEPT;

  static std::string escape(const char* s, std::size_t l) CBU_MEMORY_NOEXCEPT {
    std::string res;
    append(&res, s, l);
    return res;
  }
};

extern template struct EscapeImpl<EscapeStyle::C>;
extern template struct EscapeImpl<EscapeStyle::JSON>;
extern template struct EscapeImpl<EscapeStyle::JSON_STRICT>;

}  // namespace escape_detail

template <EscapeStyle style = EscapeStyle::C>
char* escape_string(char* w, std::string_view src) noexcept {
  return escape_detail::EscapeImpl<style>::raw(w, src.data(),
                                               src.data() + src.size());
}

template <EscapeStyle style = EscapeStyle::C>
void escape_string_append(std::string* dst,
                          std::string_view src) CBU_MEMORY_NOEXCEPT {
  return escape_detail::EscapeImpl<style>::append(dst, src.data(), src.size());
}

template <EscapeStyle style = EscapeStyle::C>
std::string escape_string(std::string_view src) CBU_MEMORY_NOEXCEPT {
  return escape_detail::EscapeImpl<style>::escape(src.data(), src.size());
}

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
