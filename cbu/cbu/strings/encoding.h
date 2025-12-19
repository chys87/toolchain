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

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <tuple>
#include <utility>

namespace cbu {

// We follow RFC 3629, allowing UTF-8 only up to 4 bytes
// (encoding Unicode up to U+10FFFF)

// Also see http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
//
//  Code Points        1st       2s       3s       4s
// U+0000..U+007F     00..7F
// U+0080..U+07FF     C2..DF   80..BF
// U+0800..U+0FFF     E0       A0..BF   80..BF
// U+1000..U+CFFF     E1..EC   80..BF   80..BF
// U+D000..U+D7FF     ED       80..9F   80..BF
// U+E000..U+FFFF     EE..EF   80..BF   80..BF
// U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
// U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
// U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
//
// Note that U+D800..U+DFFF are invalid code points.  They're used as UTF-16
// surrogate pairs.

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
  } else if (c >= 0xc2 && c <= 0xf4) {
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

// Returns true if c is a code point reserved for UTF-16 surrogate pairs
inline constexpr bool is_in_utf16_surrogate_range(std::uint32_t c) noexcept {
  return (c >= 0xd800 && c <= 0xdfff);
}

// Returns true if c is a code point reserved for UTF-16 "high" surrogate pairs
inline constexpr bool is_in_utf16_high_surrogate_range(
    std::uint32_t c) noexcept {
  return (c >= 0xd800 && c <= 0xdbff);
}

// Returns true if c is a code point reserved for UTF-16 "low" surrogate pairs
inline constexpr bool is_in_utf16_low_surrogate_range(
    std::uint32_t c) noexcept {
  return (c >= 0xdc00 && c <= 0xdfff);
}

namespace encoding_detail {

constexpr std::array<std::uint8_t, 128> make_utf8_second_byte_ranges() {
  std::array<std::uint8_t, 128> res;
  for (auto& c : res) c = 0xf0;  // All invalid
  for (auto [lo, hi, range] : std::initializer_list<
           std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>>{
           {0xc2, 0xdf, 0x8b},
           {0xe0, 0xe0, 0xab},
           {0xe1, 0xec, 0x8b},
           {0xed, 0xed, 0x89},
           {0xee, 0xef, 0x8b},
           {0xf0, 0xf0, 0x9b},
           {0xf1, 0xf3, 0x8b},
           {0xf4, 0xf4, 0x88}}) {
    for (int i = lo; i <= hi; ++i) res[i - 0x80] = range;
  }
  return res;
}

inline constexpr auto kUtf8SecondByteRanges = make_utf8_second_byte_ranges();

void utf16_surrogates_to_utf8(char8_t* dst, char16_t a, char16_t b) noexcept;

}  // namespace encoding_detail

inline constexpr bool validate_utf8_two_bytes(std::uint8_t a,
                                              std::uint8_t b) noexcept {
  if (!(a & 0x80)) return true;
  std::uint8_t range = encoding_detail::kUtf8SecondByteRanges[a - 0x80];
  return (b >= (range & 0xf0) && (b >> 4) <= (range & 0xf));
}

// If c is out of range (> U+10FFFF) or is a UTF-16 surrogate, return nullptr
char16_t* char32_to_utf16(char16_t* dst, char32_t c) noexcept;
char16_t* char32_to_utf16(char16_t* dst, const char32_t* s, size_t n) noexcept;
char8_t* char32_to_utf8(char8_t* dst, char32_t c) noexcept;
char* char32_to_utf8(char* dst, char32_t c) noexcept;

// Same as above, except that
//  * behavior is undefined if c is out of range;
//  * UTF-16 surrogates are encoded as if they were valid code points;
//  * Never returns nullptr.
char16_t* char32_to_utf16_unsafe(char16_t* dst, char32_t c) noexcept;
char16_t* char32_to_utf16_unsafe(char16_t* dst, const char32_t* s, size_t n) noexcept;
char8_t* char32_to_utf8_unsafe(char8_t* dst, char32_t c) noexcept;
char* char32_to_utf8_unsafe(char* dst, char32_t c) noexcept;

// Encode c in UTF-8, without checking whether c is a valid code point (i.e. not
// a UTF-16 surrogate).  Never returns nullptr.
char8_t* char16_to_utf8_unsafe(char8_t* dst, char16_t c) noexcept;
char* char16_to_utf8_unsafe(char* dst, char16_t c) noexcept;

char8_t* latin1_to_utf8(char8_t* dst, uint8_t) noexcept;
char* latin1_to_utf8(char* dst, uint8_t) noexcept;

// It's the caller's responsibility to guarante that (a, b) are a valid pair of
// UTF-16 surrogates.
inline constexpr char32_t utf16_surrogates_to_char32(char16_t a,
                                                     char16_t b) noexcept {
  return ((a - 0xd800) << 10) + (b - 0xdc00) + 0x10000;
}

// It's the caller's responsibility to guarante u is in [0x10000, 0x10FFFF]
inline constexpr std::pair<char16_t, char16_t>
char32_non_bmp_to_utf16_surrogates(char32_t c) noexcept {
  return {char16_t(0xd800 + (c >> 10) - (0x10000 >> 10)),
          char16_t(0xdc00 + (c & 0x3ff))};
}

char16_t* char32_non_bmp_to_utf16_surrogates(char16_t* w, char32_t c) noexcept;

// Convert a pair of UTF-16 surrogates (a, b) to UTF-8.
// It's the caller's responsibility to guarante that (a, b) are a valid pair of
// UTF-16 surrogates.  The result is guaranteed to be 4 bytes.
inline char8_t* utf16_surrogates_to_utf8(char8_t* dst, char16_t a,
                                         char16_t b) noexcept {
  encoding_detail::utf16_surrogates_to_utf8(dst, a, b);
  return dst + 4;
}

inline char* utf16_surrogates_to_utf8(char* dst, char16_t a,
                                      char16_t b) noexcept {
  return reinterpret_cast<char*>(
      utf16_surrogates_to_utf8(reinterpret_cast<char8_t*>(dst), a, b));
}

// Verify if the memory range contains valid UTF-8.
// This function makes use of AVX-2 to accelerate.
// If you want even more agressive optimizations then simdjson::validate_utf8
// is the way to go.
[[gnu::pure]] bool validate_utf8(const void* ptr, std::size_t size) noexcept;

inline bool validate_utf8(std::string_view bytes) noexcept {
  return validate_utf8(bytes.data(), bytes.size());
}

inline bool validate_utf8(std::u8string_view bytes) noexcept {
  return validate_utf8(bytes.data(), bytes.size());
}

} // namespace cbu
