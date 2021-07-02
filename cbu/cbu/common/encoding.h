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
#include <limits>

namespace cbu {

// We follow RFC 3629, allowing UTF-8 only up to 4 bytes
// (encoding Unicode up to U+10FFFF)
enum struct Utf8ByteType {
  ASCII,
  LEADING,
  TRAILING,
  INVALID,
};

inline constexpr Utf8ByteType utf8_byte_type(std::uint8_t c) noexcept {
  if (std::int8_t(c) >= 0) {
    return Utf8ByteType::ASCII;
  } else if (std::int8_t(c) < std::int8_t(0xc0) /* c >= 0x80 && c < 0xc0 */ ) {
    return Utf8ByteType::TRAILING;
  } else if (c >= 0xc0 && c < 0xf4) {
    return Utf8ByteType::LEADING;
  } else {
    return Utf8ByteType::INVALID;
  }
}


// What Unicode code point ranges can be expressed in k bytes?
inline constexpr std::uint32_t utf8_bytes_limit[4] = {
  0x0080, 0x0800, 0x10000, 0x110000
};

inline constexpr unsigned int char32_to_utf8_length(char32_t c) noexcept {
  for (std::size_t i = 0;
       i < sizeof(utf8_bytes_limit) / sizeof(utf8_bytes_limit[0]); ++i) {
    if (std::uint32_t(c) < utf8_bytes_limit[i]) {
      return i + 1;
    }
  }
  return 0;
}

inline constexpr unsigned int utf8_leading_byte_to_trailing_length(
    std::uint8_t c) noexcept {
  return (__builtin_clz(static_cast<unsigned int>(std::uint8_t(~c))) -
          (std::numeric_limits<unsigned int>::digits - 7));
}

// If c is out of range (> U+10FFFF) or is a UTF-16 surrogate, return nullptr
char8_t* char32_to_utf8(char8_t* dst, char32_t c) noexcept;
char* char32_to_utf8(char* dst, char32_t c) noexcept;

} // namespace cbu
