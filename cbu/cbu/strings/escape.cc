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

#include "cbu/strings/escape.h"

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#ifdef __ARM_NEON
# include <arm_neon.h>
#endif
#include <array>
#include <cstdint>
#include <optional>

#include "cbu/common/bit.h"
#include "cbu/common/hint.h"
#include "cbu/math/common.h"
#include "cbu/strings/encoding.h"
#include "cbu/strings/faststr.h"

namespace cbu {
namespace escape_detail {
namespace {

template <EscapeStyle style>
inline constexpr char* escape_char(char* w, std::uint8_t c) {
  if (c < 0x20) {
    static_assert('\a' == 7);
    static_assert('\b' == 8);
    static_assert('\t' == 9);
    static_assert('\n' == 10);
    static_assert('\v' == 11);
    static_assert('\f' == 12);
    static_assert('\r' == 13);
    // JSON doesn't support '\a' or '\v'
    if (style == EscapeStyle::C && unsigned(c - 7) <= 6) {
      memcpy(w, &R"(\a\b\t\n\v\f\r)"[unsigned(c - 7) * 2], 2);
      w += 2;
    } else if (style != EscapeStyle::C && c != 11 && unsigned(c - 8) <= 5) {
      memcpy(w, &R"(\b\t\n\v\f\r)"[unsigned(c - 8) * 2], 2);
      w += 2;
    } else {
      if constexpr (style == EscapeStyle::JSON ||
                    style == EscapeStyle::JSON_STRICT) {
        memcpy(w, "\\u00", 4);
        w += 4;
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

template <EscapeStyle style>
inline constexpr bool needs_escaping(std::uint8_t c) noexcept {
  return (c < 0x20 || c == '\\' || c == '\"' ||
          (style == EscapeStyle::JSON_STRICT && c == '/'));
}

template <EscapeStyle style>
constexpr char* escape_string_naive(char* w, const char* s, std::size_t n) {
  for (char c : std::string_view(s, n)) {
    if (needs_escaping<style>(c)) {
      w = escape_char<style>(w, c);
    } else {
      *w++ = c;
    }
  }
  return w;
}

#ifdef __AVX2__
template <EscapeStyle style>
[[gnu::always_inline]] inline __m256i get_encoding_mask(
    __m256i chars) noexcept {
  __v32qu vq = __v32qu(chars);
  __v32qu mask = __v32qu((vq < 0x20) | (vq == '\\') | (vq == '\"'));
  if constexpr (style == EscapeStyle::JSON_STRICT) {
    mask |= __v32qu(vq == '/');
  }
  return __m256i(mask);
}

template <EscapeStyle style>
[[gnu::always_inline]] inline char* encode_by_mask(
    char* w, const char* s, std::uint32_t bmsk, std::uint32_t from = 0,
    std::uint32_t until = 32) noexcept {
  bmsk &= (-1u << from);
  s += from;
  if (until < 32) {
    bmsk = bzhi(bmsk, until);
  }
  for (std::uint32_t offset : set_bits(bmsk)) {
    CBU_NAIVE_LOOP
    while (from < offset) {
      *w++ = *s++;
      ++from;
    }
    w = escape_char<style>(w, *s++);
    ++from;
  }
  CBU_NAIVE_LOOP
  while (from < until) {
    *w++ = *s++;
    ++from;
  }
  return w;
}
#endif // __AVX2__

#ifdef __ARM_NEON
template <EscapeStyle style>
[[gnu::always_inline]] inline uint8x16_t get_encoding_mask(
    uint8x16_t chars) noexcept {
  uint8x16_t mask = uint8x16_t((chars < 0x20) | (chars == '\\') | (chars == '\"'));
  if constexpr (style == EscapeStyle::JSON_STRICT) {
    mask |= uint8x16_t(chars == '/');
  }
  return mask;
}

template <EscapeStyle style>
[[gnu::always_inline]] inline char* encode_by_mask(
    char* w, const char* s, std::uint64_t bmsk, std::uint32_t from = 0,
    std::uint32_t until = 16) noexcept {
  s += from;

  from *= 4;
  until *= 4;

  bmsk &= (std::uint64_t(-1) << from);
  if (until < 16 * 4) {
    bmsk = bzhi(bmsk, until);
  }
  for (uint32_t offset : set_bits(bmsk & 0x1111'1111'1111'1111ull)) {
    CBU_NAIVE_LOOP
    while (from != offset) {
      *w++ = *s++;
      from += 4;
    }
    w = escape_char<style>(w, *s++);
    from += 4;
  }
  CBU_NAIVE_LOOP
  while (from != until) {
    *w++ = *s++;
    from += 4;
  }
  return w;
}
#endif  // __ARM_NEON

} // namespace

template <EscapeStyle style>
char* EscapeImpl<style>::raw(char* w, const char* s, std::size_t n) noexcept {
#if defined __AVX2__ && !defined CBU_ADDRESS_SANITIZER
  if (n == 0) {
    return w;
  }

  std::size_t misalign = std::uintptr_t(s) & 31;
  s = (const char*)(std::uintptr_t(s) & -32);
  n += misalign;
  __m256i chars = *(const __m256i*)s;
  __m256i msk = get_encoding_mask<style>(chars);
  std::uint32_t bmsk = _mm256_movemask_epi8(msk);
  w = encode_by_mask<style>(w, s, bmsk, misalign,
                            std::min<std::uint32_t>(32, n));
  s += 32;
  if (n <= 32) {
    return w;
  }
  n -= 32;

  while (n >= 32) {
    __m256i chars = *(const __m256i*)s;
    __m256i msk = get_encoding_mask<style>(chars);
    if (_mm256_testz_si256(msk, msk)) {
      *(__m256i_u*)w = chars;
      w += 32;
    } else {
      std::uint32_t bmsk = _mm256_movemask_epi8(msk);
      CBU_HINT_ASSUME(bmsk != 0);
      w = encode_by_mask<style>(w, s, bmsk);
    }
    s += 32;
    n -= 32;
  }

  if (n) {
    __m256i chars = *(const __m256i*)s;
    __m256i msk = get_encoding_mask<style>(chars);
    std::uint32_t bmsk = _mm256_movemask_epi8(msk);
    w = encode_by_mask<style>(w, s, bmsk, 0, n);
  }
  return w;
#endif // __AVX2__
#if defined __ARM_NEON && !defined CBU_ADDRESS_SANITIZER
  if (n == 0) return w;

  std::size_t misalign = std::uintptr_t(s) & 15;
  s = (const char*)(std::uintptr_t(s) & -16);
  n += misalign;
  uint8x16_t chars = *(const uint8x16_t*)s;
  uint8x16_t msk = get_encoding_mask<style>(chars);
  std::uint64_t bmsk = uint64x1_t(vshrn_n_u16(uint16x8_t(msk), 4))[0];
  w = encode_by_mask<style>(w, s, bmsk, misalign,
                            std::min<std::uint32_t>(16, n));
  s += 16;
  if (n <= 16) return w;
  n -= 16;

  while (n >= 16) {
    uint8x16_t chars = *(const uint8x16_t*)s;
    uint8x16_t msk = get_encoding_mask<style>(chars);
    std::uint64_t bmsk = uint64x1_t(vshrn_n_u16(uint16x8_t(msk), 4))[0];
    if (bmsk == 0) {
      *(uint8x16_t*)w = chars;
      w += 16;
    } else {
      CBU_HINT_ASSUME((bmsk & 0x1111'1111'1111'1111ull) != 0);
      w = encode_by_mask<style>(w, s, bmsk);
    }
    s += 16;
    n -= 16;
  }

  if (n) {
    uint8x16_t chars = *(const uint8x16_t*)s;
    uint8x16_t msk = get_encoding_mask<style>(chars);
    std::uint64_t bmsk = uint64x1_t(vshrn_n_u16(uint16x8_t(msk), 4))[0];
    w = encode_by_mask<style>(w, s, bmsk, 0, n);
  }
  return w;
#endif // __ARM_NEON
  return escape_string_naive<style>(w, s, n);
}

template <EscapeStyle style>
void EscapeImpl<style>::append(std::string* dst, const char* s,
                               std::size_t n) CBU_MEMORY_NOEXCEPT {
  char* p = extend(
      dst, (style == EscapeStyle::JSON || style == EscapeStyle::JSON_STRICT)
               ? 6 * n
               : 4 * n);
  p = raw(p, s, n);
  truncate_unsafe(dst, p - dst->data());
}

template struct EscapeImpl<EscapeStyle::C>;
template struct EscapeImpl<EscapeStyle::JSON>;
template struct EscapeImpl<EscapeStyle::JSON_STRICT>;

}  // namespace escape_detail

namespace {

inline constexpr std::tuple<UnescapeStringStatus, char*, const char*> tuplize(
    const UnescapeStringResult& result) noexcept {
  return {result.status, result.dst_ptr, result.src_ptr};
}

std::tuple<char, char*, const char*> copy_until_backslash(
    char* dst, const char* src, const char* end) noexcept {
#ifdef __AVX2__
  while (src + 32 <= end) {
    __v32qu val = __v32qu(*(const __m256i_u*)src);
    __m256i mask = __m256i((val == '\\') | (val == '\"'));
    if (_mm256_testz_si256(mask, mask)) {
      *(__m256i_u*)dst = __m256i(val);
      src += 32;
      dst += 32;
    } else {
      unsigned off = ctz(_mm256_movemask_epi8(mask));
      CBU_NAIVE_LOOP
      for (unsigned i = off; i; --i) *dst++ = *src++;
      return {*src, dst, src};
    }
  }
#elifdef __ARM_NEON
  while (src + 16 <= end) {
    uint8x16_t val = *(const uint8x16_t*)src;
    uint8x16_t mask = uint8x16_t((val == '\\') || (val == '"'));
    if (vmaxvq_u8(mask) == 0) {
      *(uint8x16_t*)dst = val;
      src += 16;
      dst += 16;
    } else {
      unsigned off = ctz(uint64x1_t(vshrn_n_u16(uint16x8_t(mask), 4))[0]) / 4;
      CBU_NAIVE_LOOP
      for (unsigned i = off; i; --i) *dst++ = *src++;
      return {*src, dst, src};
    }
  }
#endif
  while (src < end && *src != '\\' && *src != '\"') {
    *dst++ = *src++;
  }
  if (src >= end) {
    return {'\0', dst, src};
  } else {
    return {*src, dst, src};
  }
}

inline consteval std::array<char, 128> make_unescape_fast_map() noexcept {
  std::array<char, 128> res{};
  res['\\'] = '\\';
  res['\"'] = '\"';
  res['\''] = '\'';
  res['/'] = '/';  // For JSON
  res['a'] = '\a';
  res['b'] = '\b';
  res['e'] = '\x1f';
  res['f'] = '\f';
  res['n'] = '\n';
  res['r'] = '\r';
  res['t'] = '\t';
  res['v'] = '\v';
  return res;
}

inline constexpr std::array<char, 128> UNESCAPE_FAST_MAP =
    make_unescape_fast_map();

UnescapeStringResult parse_escape_sequence(
    char* dst, const char* src, const char* end) noexcept {
  const char* start_src = src - 1;
  if (src >= end) return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
  std::uint8_t c = *src++;
  char replacement = 0;
  if (c < 0x80) replacement = UNESCAPE_FAST_MAP[c];
  if (replacement != 0) {
    *dst++ = replacement;
    return {UnescapeStringStatus::OK_QUOTE, dst, src};
  } else if (c == 'x') {
    if (src + 2 > end)
      return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
    auto r = convert_2xdigit(src);
    if (!r)
      return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
    src += 2;
    *dst++ = *r;
    return {UnescapeStringStatus::OK_QUOTE, dst, src};
  } else if (c >= '0' && c <= '7') {
    // Various parsers disagree how to parse octal representations
    // overflowing octet.
    // E.g., "\400" is " 0" in Node.js and is "\0" in Python.
    // We take the Node.js approach since it appears more reasonable.
    unsigned ch = c - '0';
    if (src < end && (*src >= '0' && *src <= '7')) {
      ch = ch * 8 + (*src++ - '0');
      if ((ch < 040) && src < end && (*src >= '0' && *src <= '7')) {
        ch = ch * 8 + (*src++ - '0');
      }
    }
    *dst++ = ch;
    return {UnescapeStringStatus::OK_QUOTE, dst, src};
  } else {
    char32_t ch;
    if (c == 'u') {
      if (src + 4 > end)
        return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
      auto r = convert_4xdigit(src);
      if (!r) return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
      src += 4;

      ch = *r;
      if ((ch >= 0xd800) && (ch <= 0xdbff)) {
        // UTF-16 lead surrogate
        if (src + 6 > end || src[0] != '\\' || src[1] != 'u')
          return {UnescapeStringStatus::HEAD_SURROGATE_WITHOUT_TAIL,
                  dst, start_src};
        r = convert_4xdigit(src + 2);
        if (!r || !(*r >= 0xdc00 && *r <= 0xdfff))
          return {UnescapeStringStatus::HEAD_SURROGATE_WITHOUT_TAIL,
                  dst, start_src};
        src += 6;
        ch = 0x10000 + (ch - 0xd800) * 1024 + (*r - 0xdc00);
      } else if ((ch >= 0xdc00) && (ch <= 0xdfff)) {
        // UTF-16 tail surrogate
        return {UnescapeStringStatus::TAIL_SURROGATE_WITHOUT_HEAD,
                dst, start_src};
      }
    } else if (c == 'U') {
      if (src + 8 > end)
        return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
      auto r = convert_8xdigit(src);
      if (!r) return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
      src += 8;
      ch = *r;
    } else {
      return {UnescapeStringStatus::INVALID_ESCAPE, dst, start_src};
    }

    char* new_dst = char32_to_utf8(dst, ch);
    if (new_dst == nullptr)
      return {UnescapeStringStatus::CODE_POINT_OUT_OF_RANGE, dst, start_src};
    dst = new_dst;
    return {UnescapeStringStatus::OK_QUOTE, dst, src};
  }
}

} // namespace

UnescapeStringResult unescape_string(char* dst, const char* src,
                                     const char* end) noexcept {
  for (;;) {
    char c;
    std::tie(c, dst, src) = copy_until_backslash(dst, src, end);
    if (src >= end) return {UnescapeStringStatus::OK_EOS, dst, src};
    if (c == '\"')
      return {UnescapeStringStatus::OK_QUOTE, dst, src};
    // Now c (a.k.a. *src) must be '\\'
    ++src;
    UnescapeStringStatus status;
    std::tie(status, dst, src) = tuplize(parse_escape_sequence(dst, src, end));
    if (status != UnescapeStringStatus::OK_QUOTE)
      return {status, dst, src};
  }
}

} // namespace cbu
