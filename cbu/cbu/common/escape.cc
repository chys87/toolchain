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

#include "cbu/common/escape.h"

#if __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#include <array>
#include <cstdint>

#include "cbu/common/bit.h"
#include "cbu/common/fastarith.h"
#include "cbu/common/faststr.h"

namespace cbu {
inline namespace cbu_escape {
namespace {

inline consteval std::array<char, 32> make_special_escape_map() {
  std::array<char, 32> res{};
  // '\v' is not universally accepted.
  res['\b'] = 'b';
  res['\f'] = 'f';
  res['\n'] = 'n';
  res['\r'] = 'r';
  res['\t'] = 't';
  return res;
}

constexpr std::array<char, 32> SPECIAL_ESCAPE_MAP = make_special_escape_map();

inline constexpr char* escape_char(char* w, std::uint8_t c, EscapeStyle style) {
  if (c < 0x20) {
    if (char special = SPECIAL_ESCAPE_MAP[c]) {
      *w++ = '\\';
      *w++ = special;
    } else {
      if (style == EscapeStyle::JSON) {
        *w++ = '\\';
        *w++ = 'u';
        *w++ = '0';
        *w++ = '0';
      } else {
        *w++ = '\\';
        *w++ = 'x';
      }
      *w++ = "0123456789abcdef"[c >> 4];
      *w++ = "0123456789abcdef"[c & 15];
    }
  } else {
    *w++ = '\\';
    *w++ = c;
  }
  return w;
}

inline constexpr bool needs_escaping(std::uint8_t c,
                                     EscapeStyle style) noexcept {
  return (c < 0x20 || c == '\\' || c == '\"' ||
          (c == '/' && style == EscapeStyle::JSON));
}

constexpr char* escape_string_naive(
    char* w, std::string_view src, EscapeStyle style) {
  for (char c : src) {
    if (needs_escaping(c, style)) {
      w = escape_char(w, c, style);
    } else {
      *w++ = c;
    }
  }
  return w;
}

#ifdef __AVX2__
inline constexpr __v32qu v32qu(std::uint8_t c) noexcept {
  return __v32qu{
    c, c, c, c, c, c, c, c,
    c, c, c, c, c, c, c, c,
    c, c, c, c, c, c, c, c,
    c, c, c, c, c, c, c, c};
}

inline __m256i get_encoding_mask(__m256i chars, EscapeStyle style) noexcept {
  __v32qu vq = __v32qu(chars);
  __v32qu mask = ((vq < v32qu(0x20)) | (vq == v32qu('\\')) |
                  (vq == v32qu('\"')));
  if (style == EscapeStyle::JSON) {
    mask |= (vq == v32qu('/'));
  }
  return __m256i(mask);
}

inline char* encode_by_mask(char* w, const char* s,
                            std::uint32_t bmsk,
                            EscapeStyle style,
                            std::uint32_t from = 0,
                            std::uint32_t until = 32) noexcept {
  bmsk &= (-1u << from);
  s += from;
  if (until < 32) {
    bmsk = bzhi(bmsk, until);
  }
  for (std::uint32_t offset : set_bits(bmsk)) {
    while (from < offset) {
      *w++ = *s++;
      ++from;
    }
    w = escape_char(w, *s++, style);
    ++from;
  }
  while (from < until) {
    *w++ = *s++;
    ++from;
  }
  return w;
}
#endif // __AVX2__

} // namespace

char* escape_string(char* w, std::string_view src, EscapeStyle style)
    noexcept {
#ifdef __AVX2__
  if (src.empty()) {
    return w;
  }

  const char* s = src.data();
  std::size_t n = src.size();
  std::size_t misalign = std::uintptr_t(s) & 31;
  s = (const char*)(std::uintptr_t(s) & -32);
  n += misalign;
  __m256i chars = *(const __m256i*)s;
  __m256i msk = get_encoding_mask(chars, style);
  std::uint32_t bmsk = _mm256_movemask_epi8(msk);
  w = encode_by_mask(w, s, bmsk, style,
                     misalign, std::min<std::uint32_t>(32, n));
  s += 32;
  if (n <= 32) {
    return w;
  }
  n -= 32;

  while (n & -32) {
    __m256i chars = *(const __m256i*)s;
    __m256i msk = get_encoding_mask(chars, style);
    if (_mm256_testz_si256(msk, msk)) {
      *(__m256i_u*)w = chars;
      w += 32;
    } else {
      w = encode_by_mask(w, s, _mm256_movemask_epi8(msk), style);
    }
    s += 32;
    n -= 32;
  }

  if (n) {
    __m256i chars = *(const __m256i*)s;
    __m256i msk = get_encoding_mask(chars, style);
    std::uint32_t bmsk = _mm256_movemask_epi8(msk);
    w = encode_by_mask(w, s, bmsk, style, 0, n);
  }
  return w;
#endif // __AVX2__
  return escape_string_naive(w, src, style);
}

void escape_string_append(std::string* dst,
                          std::string_view src, EscapeStyle style) {
  if (!src.empty()) {
    char* p = extend(
        dst, (style == EscapeStyle::JSON) ? 6 * src.size() : 4 * src.size());
    p = escape_string(p, src, style);
    truncate_unsafe(dst, p - dst->data());
  }
}

std::string escape_string(std::string_view src, EscapeStyle style) {
  std::string res;
  escape_string_append(&res, src, style);
  return res;
}

} // inline namespace cbu_escape
} // namespace cbu
