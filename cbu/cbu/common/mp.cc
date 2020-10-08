/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2020, chys <admin@CHYS.INFO>
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

#include "mp.h"
#include <string.h>
#include <algorithm>
#include <limits>
#include <tuple>
#if __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#include "bit.h"
#include "fastarith.h"
#include "faststr.h"
#include "byteorder.h"
#include "strutil.h"

namespace cbu {
namespace mp {
namespace {

inline unsigned convert_xdigit(unsigned char c, bool *success = nullptr) {
  if ((c >= '0') && (c <= '9')) {
    if (success)
      *success = true;
    return (c - '0');
  } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
    if (success)
      *success = true;
    return (((c | 0x20) - 'a') + 10);
  } else {
    if (success)
      *success = false;
    return -1u;
  }
}

// r = a * b + c
inline size_t Madd(Word *r, const Word *a, size_t na, Word b, Word c) noexcept {
  if (na == 0 || b == 0) {
    if (c == 0) {
      return 0;
    } else {
      *r = c;
      return 1;
    }
  }

  if (b == 1) {
    if (c == 0) {
      if (r != a)
        memcpy(r, a, na * sizeof(Word));
      return na;
    }

    if (!add_overflow(*a, c, r)) {
      if (r != a)
        memcpy(r + 1, a + 1, (na - 1) * sizeof(Word));
      return na;
    } else {
      size_t k;
      for (k = 1; k < na; ++k) {
        if (!add_overflow(a[k], Word(1), &r[k])) {
          if (r != a)
            memcpy(r + k + 1, a + k + 1, (na - k - 1) * sizeof(Word));
          return na;
        }
      }
      r[na] = 1;
      return na + 1;
    }
  }

  for (size_t k = 0; k < na; ++k) {
    DWord t = DWord(a[k]) * b + c;
    r[k] = Word(t);
    c = Word(t >> (8 * sizeof(Word)));
  }

  if (c) {
    r[na] = c;
    ++na;
  }
  return na;
}

// Caller guarantees the quotient is representible by Word.
// Otherwise, it's undefined behavior
inline std::pair<Word, Word> Div(Word hi, Word lo, Word b) noexcept {
#if defined __x86_64__
  Word quo, rem;
  asm ("div %[b]" : "=a"(quo), "=d"(rem) : "a"(lo), "d"(hi), [b]"r"(b) : "cc");
  return std::make_pair(quo, rem);
#endif
  DWord a = DWord(hi) << (8 * sizeof(Word)) | lo;
  return std::make_pair(a / b, a % b);
}

inline Word add_carry(Word a, Word b, Word c, Word *res) noexcept {
  Word tmp;
  bool c1 = add_overflow(a, b, &tmp);
  bool c2 = add_overflow(tmp, c, res);
  return (c1 || c2);
}

inline std::pair<size_t, Word> Div(Word *r, const Word *a, size_t na,
                                   Word b) noexcept {
  Word c = 0;

  size_t k = na;
  for (; k; --k) {
    Word quo;
    std::tie(quo, c) = Div(c, a[k - 1], b);
    if (quo != 0) {
      r[k - 1] = quo;
      size_t rn = k;
      while (--k)
        std::tie(r[k - 1], c) = Div(c, a[k - 1], b);
      return std::make_pair(rn, c);
    }
  }
  return std::make_pair(0, c);
}

} // namespace

size_t mul(Word *r, const Word *a, size_t na, Word b) noexcept {
  return Madd(r, a, na, b, 0);
}

size_t mul(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept {
  if (na < nb) {
    std::swap(a, b);
    std::swap(na, nb);
  }
  if (nb == 0) {
    return 0;
  }
  if (nb == 1) {
    return mul(r, a, na, b[0]);
  }

  Word restmp[na + nb];
  Word multmp[na + 1];

  size_t nr = 0;
  for (size_t i = 0; i < nb; ++i) {
    size_t nm = mul(multmp, a, na, b[i]);
    nr = i + add(restmp + i, restmp + i, nr - i, multmp, nm);
  }
  memcpy(r, restmp, nr * sizeof(Word));
  return nr;
}

size_t add(Word *r, const Word *a, size_t na, Word b) noexcept {
  return Madd(r, a, na, 1, b);
}

size_t add(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept {
  if (na < nb) {
    std::swap(a, b);
    std::swap(na, nb);
  }
  size_t nr = 0;
  Word carry = 0;
  for (; nb; --nb, --na) {
    carry = add_carry(*a++, *b++, carry, &r[nr++]);
  }
  if (na > 0) {
    nr += Madd(r + nr, a, na, 1, carry);
  } else if (carry) {
    r[nr++] = 1;
  }
  return nr;
}

std::pair<size_t, Word> div(Word *r, const Word *a, size_t na, Word b) noexcept {
  return Div(r, a, na, b);
}

int compare(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  na = minimize(a, na);
  nb = minimize(b, nb);
  if (na > nb) {
    return 1;
  } else if (na < nb) {
    return -1;
  } else {
    for (size_t k = na; k; --k) {
      if (a[k - 1] != b[k - 1]) {
        if (a[k - 1] > b[k - 1]) {
          return 1;
        } else {
          return -1;
        }
      }
    }
    return 0;
  }
}

bool eq(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  na = minimize(a, na);
  nb = minimize(b, nb);
  if (na != nb) {
    return false;
  }
  while (na--) {
    if (*a++ != *b++) {
      return false;
    }
  }
  return true;
}

bool ne(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  return !eq(a, na, b, nb);
}

bool gt(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  return compare(a, na, b, nb) > 0;
}

bool lt(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  return compare(a, na, b, nb) < 0;
}

bool ge(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  return compare(a, na, b, nb) >= 0;
}

bool le(const Word *a, size_t na, const Word *b, size_t nb) noexcept {
  return compare(a, na, b, nb) < 0;
}

size_t from_dec(Word *r, const char *s, size_t n) noexcept {
  size_t rn = 0;

  while (n >= 8) {
    uint32_t v =
      ((s[0] - '0') * 10 +
       (s[1] - '0')) * 100 +
      ((s[2] - '0') * 10 +
       (s[3] - '0'));
    v = v * 10000 +
      ((s[4] - '0') * 10 +
       (s[5] - '0')) * 100 +
      ((s[6] - '0') * 10 +
       (s[7] - '0'));

    rn = Madd(r, r, rn, 100000000, v);

    n -= 8;
    s += 8;
  }

  if (n) {
    uint32_t v = 0;
    uint32_t scale = 1;
    for (unsigned k = n; k; --k) {
      scale *= 10;
      v = v * 10 + uint8_t(*s++) - '0';
    }
    rn = Madd(r, r, rn, scale, v);
  }

  return rn;
}

char *to_dec(char *r, const Word *s, size_t n) noexcept {
  n = minimize(s, n);
  if (n == 0) {
    *r++ = '0';
    return r;
  }

  char *w = r;

  Word t[n];
  memcpy(t, s, n * sizeof(Word));

  while (n > 1) {
    uint32_t rem;
    std::tie(n, rem) = Div(t, t, n, 10000);
    uint32_t hi = rem / 100;
    uint32_t lo = rem % 100;
    *w++ = (lo % 10) + '0';
    *w++ = (lo / 10) + '0';
    *w++ = (hi % 10) + '0';
    *w++ = (hi / 10) + '0';
  }

  Word v = t[0];
  do {
    *w++ = v % 10 + '0';
    v /= 10;
  } while (v);

  return reverse(r, w);
}

std::string to_dec(const Word *s, size_t n) noexcept {
  std::string res;
  res.resize(n * std::numeric_limits<Word>::digits10 + 1);
  char *r = to_dec(&res[0], s, n);
  res.resize(r - &res[0]);
  return res;
}

size_t from_hex(Word *r, const char *s, size_t n) noexcept {
  while (n && *s == '0')
    --n, ++s;

  if (n == 0)
    return 0;

  size_t i = 0;
  while (n > 2 * sizeof(Word)) {
    n -= 2 * sizeof(Word);
    Word word = 0;
    const char *p = s + n;
    for (unsigned k = 2 * sizeof(Word); k; --k)
      word = word * 16 + convert_xdigit(*p++);
    r[i++] = word;
  }

  Word word = 0;
  for (; n; --n)
    word = word * 16 + convert_xdigit(*s++);
  r[i++] = word;
  return i;
}

char *to_hex(char *r, const Word *s, size_t n) noexcept {
  const char *hex = "0123456789abcdef";

#if __BYTE_ORDER != __LITTLE_ENDIAN
  Word cp[n];
  for (size_t i = 0; i < n; ++i)
    cp[i] = bswap(s[i]);
  s = cp;
#endif

  const uint8_t *b = reinterpret_cast<const uint8_t *>(s);
  size_t bytes = n * sizeof(Word);
  const uint8_t *scan = b + bytes - 1;

  while (scan >= b && *scan == 0)
    --scan;

  if (scan < b) {
    *r++ = '0';
    return r;
  }

  if (!(*scan & 0xf0)) {
    *r++ = hex[*scan];
    --scan;
  }

  while (scan >= b) {
    uint8_t byte = *scan--;
    *r++ = hex[byte >> 4];
    *r++ = hex[byte & 0xf];
  }

  return r;
}

std::string to_hex(const Word *s, size_t n) noexcept {
  std::string res;
  res.resize(n * (std::numeric_limits<Word>::digits / 4) + 1);
  char *r = to_hex(&res[0], s, n);
  res.resize(r - &res[0]);
  return res;
}

size_t from_oct(Word *r, const char *s, size_t n) noexcept {
  while (n && *s == '0')
    --n, ++s;

  size_t rn = 0;

  for (; n; --n)
    rn = Madd(r, r, rn, 8, uint8_t(*s++) - '0');

  return rn;
}

char *to_oct(char *r, const Word *s, size_t n) noexcept {
  n = minimize(s, n);
  if (n == 0) {
    *r++ = '0';
    return r;
  }

#if __BYTE_ORDER != __LITTLE_ENDIAN
  Word cp[n];
  for (size_t i = 0; i < n; ++i)
    cp[i] = bswap(s[i]);
  s = cp;
#endif

  const uint8_t *b = reinterpret_cast<const uint8_t *>(s);
  size_t bytes = minimize(b, n * sizeof(Word));

  const uint8_t *pb = b + bytes / 3 * 3;
  unsigned tail = bytes % 3;

  unsigned v;
  switch (tail) {
    case 0:
      pb -= 3;
      v = mempick_le<uint16_t>(pb) | pb[2] << 16;
      break;
    case 1:
      v = pb[0];
      break;
    case 2:
      v = mempick_le<uint16_t>(pb);
  }

  unsigned tail_oct_digits = bsr(v) / 3 + 1;

#if defined __x86_64__ && defined __BMI2__
  uint64_t bv = _pdep_u64(v, 0x0707070707070707) + 0x3030303030303030;
  bv = bswap(bv) >> (64 - tail_oct_digits * 8);
  r = memdrop(r, bv, tail_oct_digits);
#else
  unsigned bits = tail_oct_digits * 3;
  while (bits) {
    bits -= 3;
    *r++ = (v >> bits) + '0';
    v = bzhi(v, bits);
  }
#endif

  while (pb > b) {
    pb -= 3;
    unsigned v = mempick_le<uint32_t>(pb);
#if defined __x86_64__ && defined __BMI2__
    uint64_t bv = _pdep_u64(v, 0x0707070707070707) + 0x3030303030303030;
    r = memdrop_be(r, bv);
#else
    unsigned b0 = pb[0], b1 = pb[1], b2 = pb[2];
    *r++ = (b2 >> 5) + '0';
    *r++ = (7 & (b2 >> 2)) + '0';
    *r++ = (7 & (v >> 15)) + '0';
    *r++ = (7 & (b1 >> 4)) + '0';
    *r++ = (7 & (b1 >> 1)) + '0';
    *r++ = (7 & (v >> 6)) + '0';
    *r++ = (7 & (b0 >> 3)) + '0';
    *r++ = (7 & (b0 >> 0)) + '0';
#endif
  }

  return r;
}

std::string to_oct(const Word *s, size_t n) noexcept {
  std::string res;
  res.resize(n * std::numeric_limits<Word>::digits / 3 + 1);
  char *r = to_oct(&res[0], s, n);
  res.resize(r - &res[0]);
  return res;
}

size_t from_bin(Word *r, const char *s, size_t n) noexcept {
  while (n && *s == '0')
    --n, ++s;

  if (n == 0)
    return 0;

  size_t nr = 0;
  while (n > 8 * sizeof(Word)) {
    n -= 8 * sizeof(Word);
    const char *p = s + n;
#if defined __x86_64__ && defined __AVX2__
    static_assert(sizeof(Word) == 8);
    __m256i va = _mm256_slli_epi16(*reinterpret_cast<const __m256i_u *>(p), 7);
    __m256i vb = _mm256_slli_epi16(*reinterpret_cast<const __m256i_u *>(p + 32),
                                   7);
    Word v = uint32_t(_mm256_movemask_epi8(va)) |
             uint64_t(uint32_t(_mm256_movemask_epi8(vb))) << 32;
#else
    Word v = 0;
    for (unsigned k = 8 * sizeof(Word); k; --k)
      v = v * 2 + *p++ - '0';
#endif
    r[nr++] = v;
  }

  Word v = 0;
  for (unsigned k = n; k; --k)
    v = v * 2 + *s++ - '0';
  r[nr++] = v;

  return nr;
}

char *to_bin(char *r, const Word *s, size_t n) noexcept {
  n = minimize(s, n);
  if (n == 0) {
    *r++ = '0';
    return r;
  }

  Word v = s[n - 1];
  unsigned bits = bsr(v) + 1;
  do {
    --bits;
    *r++ = (v >> bits) + '0';
    v = bzhi(v, bits);
  } while (bits);

  while (--n) {
    Word v = s[n - 1];
    for (unsigned k = sizeof(v); k; --k) {
      uint8_t b = v >> ((k - 1) * 8);
#if defined __AVX2__ && defined __x86_64__
      __m128i u = _mm_mullo_epi16(_mm_set1_epi16(b),
                                  _mm_setr_epi16(1, 2, 4, 8, 16, 32, 64, 128));
      u = _mm_srli_epi16(u, 7) & _mm_set1_epi16(1);
      u = _mm_add_epi8(_mm_packus_epi16(u, u), _mm_set1_epi8('0'));
      r = memdrop8(r, _mm_cvtsi128_si64x(u));
#else
      *r++ = (b >> 7) + '0';
      *r++ = (1 & (b >> 6)) + '0';
      *r++ = (1 & (b >> 5)) + '0';
      *r++ = (1 & (b >> 4)) + '0';
      *r++ = (1 & (b >> 3)) + '0';
      *r++ = (1 & (b >> 2)) + '0';
      *r++ = (1 & (b >> 1)) + '0';
      *r++ = (1 & (b >> 0)) + '0';
#endif
    }
  }

  return r;
}

std::string to_bin(const Word *s, size_t n) noexcept {
  std::string res;
  res.resize(n * std::numeric_limits<Word>::digits + 1);
  char *r = to_bin(&res[0], s, n);
  res.resize(r - &res[0]);
  return res;
}

} // namespace mp
} // namespace cbu
