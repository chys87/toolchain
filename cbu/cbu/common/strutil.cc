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

#include "cbu/common/strutil.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <type_traits>
#if __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif

#include "cbu/common/bit.h"
#include "cbu/common/byteorder.h"
#include "cbu/common/encoding.h"
#include "cbu/common/fastarith.h"
#include "cbu/common/faststr.h"

namespace cbu {

using std::uint32_t;
using std::uint64_t;
using std::uintptr_t;
using std::intptr_t;
using std::size_t;
using ssize_t = std::make_signed_t<size_t>;

std::size_t vscnprintf(char *buf, std::size_t bufsiz, const char *format,
                       std::va_list ap) noexcept {
  // Be careful we should be able to handle bufsiz == 0
  ssize_t len = std::vsnprintf(buf, bufsiz, format, ap);
  if (len >= ssize_t(bufsiz)) {
    len = bufsiz - 1;
  }
  if (len < 0) {
    len = 0;
  }
  return len;
}

std::size_t scnprintf(char *buf, std::size_t bufsiz,
                      const char *format, ...) noexcept {
  std::size_t r;
  std::va_list ap;
  va_start(ap, format);
  r = vscnprintf(buf, bufsiz, format, ap);
  va_end(ap);
  return r;
}

char *strdup_new(const char *s) {
  std::size_t l = std::strlen(s) + 1;
  return static_cast<char *>(std::memcpy(new char[l], s, l));
}

std::size_t strcnt(const char *s, char c) noexcept {
  if (c == 0) {
    return 0;
  }
  size_t r = 0;
#if defined __AVX2__
  size_t misalign = (uintptr_t)s & 31;
  __m256i ref = _mm256_set1_epi8(c);
  __m256i zero = _mm256_setzero_si256();
  const __m256i *p = (const __m256i *)((uintptr_t)s & ~31);

  __m256i val = *p++;
  {
    uint32_t mask_z = uint32_t(_mm256_movemask_epi8(
        _mm256_cmpeq_epi8(zero, val))) >> misalign;
    uint32_t mask_c = uint32_t(_mm256_movemask_epi8(
        _mm256_cmpeq_epi8(val, ref))) >> misalign;
    if (mask_z) {
      // a ^ (a - 1): blsmsk
      mask_c &= mask_z ^ (mask_z - 1);
      return uint32_t(_mm_popcnt_u32(mask_c));
    }
    r = uint32_t(_mm_popcnt_u32(mask_c));
  }

  for (;;) {
    val = *p++;
    __m256i z = _mm256_cmpeq_epi8(zero, val);
    uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(ref, val));
    if (!_mm256_testz_si256(z, z)) {
      // null terminator found
      uint32_t mask_z = _mm256_movemask_epi8(z);
      mask &= mask_z ^ (mask_z - 1);
      r += uint32_t(_mm_popcnt_u32(mask));
      break;
    }
    r += uint32_t(_mm_popcnt_u32(mask));
  }
#else
  const char *t;
  while ((t = strchr(s, c)) != nullptr) {
    ++r;
    s = t + 1;
  }
#endif
  return r;
}

size_t memcnt(const char *s, char c, size_t n) noexcept {
  size_t r = 0;
#if defined __AVX2__ && defined __BMI2__
  if (n == 0) {
    return 0;
  }

  __m256i ref = _mm256_set1_epi8(c);
  size_t misalign = (uintptr_t)s & 31;
  const __m256i *p = (const __m256i *)((uintptr_t)s & -32);

  std::uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(ref, *p++));
  mask >>= misalign;

  size_t l = n + misalign - 32;
  if (ssize_t(l) <= 0) {
    return uint32_t(_mm_popcnt_u32(_bzhi_u32(mask, n)));
  }

  r = uint32_t(_mm_popcnt_u32(mask));

  while (!sub_overflow(l, 32, &l)) {
    r += uint32_t(_mm_popcnt_u32(_mm256_movemask_epi8(
        _mm256_cmpeq_epi8(ref, *p++))));
  }

  unsigned tail = unsigned(l) & 31;
  if (tail) {
    r += uint32_t(_mm_popcnt_u32(_bzhi_u32(_mm256_movemask_epi8(
        _mm256_cmpeq_epi8(ref, *p++)), tail)));
  }
#else
  const char *e = s + n;
  const char *t;
  while ((t = static_cast<const char *>(std::memchr(s, c, e - s))) != nullptr) {
    ++r;
    s = t + 1;
  }
#endif
  return r;
}

int strnumcmp(const char *a, const char *b) noexcept {
  const uint8_t *u = (const uint8_t *)a;
  const uint8_t *v = (const uint8_t *)b;
  unsigned U, V;

#if defined __AVX2__
  if ((uint32_t(uintptr_t(a)) | uint32_t(uintptr_t(b))) % 32 == 0) {
    // Both are aligned to 32-byte boundaries
    for (;;) {
      __m256i valu = *(const __m256i *)u;
      __m256i valv = *(const __m256i *)v;
      uint32_t ne = ~_mm256_movemask_epi8(_mm256_cmpeq_epi8(valv, valu));
      uint32_t z = _mm256_movemask_epi8(_mm256_cmpeq_epi8(
          valu, _mm256_setzero_si256()));
      if (ne | z) {
        unsigned off = ctz(ne | z);
        unsigned len = ctz(z);
        if (len < off) {
          return 0;
        }
        u += off + 1;
        v += off + 1;
        U = u[-1];
        V = v[-1];
        break;
      }
      u += 32;
      v += 32;
    }
  } else
#endif
  {
    do {
      U = *u++;
      V = *v++;
    } while ((U == V) && V);
    if (U == V) {
      return 0;
    }
  }

  int cmpresult = U - V;

  // Both are digits - Compare as numbers:
  // We use the following rule:
  // If the two series of digits are of unequal length,
  // the longer one is larger;
  // otherwise, the comparison is straightforward
  for (;;) {
    U -= '0';
    V -= '0';
    if (U < 10) {
      if (V >= 10) {
        // Only U is a digit
        return 1;
      }
    } else if (V < 10) {
      // Only V is a digit
      return -1;
    } else {
      // Neither is a digit
      return cmpresult;
    }
    U = *u++;
    V = *v++;
  }
}

char *reverse(char *p, char *q) noexcept {
  char *ret = q;
  while (p + 16 <= q) {
    uint64_t a = bswap(mempick8(p));
    uint64_t b = bswap(mempick8(q - 8));
    memdrop8(q - 8, a);
    memdrop8(p, b);
    p += 8;
    q -= 8;
  }
  if (p + 8 <= q) {
    uint32_t a = bswap(mempick4(p));
    uint32_t b = bswap(mempick4(q - 4));
    memdrop4(q - 4, a);
    memdrop4(p, b);
    p += 4;
    q -= 4;
  }
  std::reverse(p, q);
  return ret;
}

int compare_string_view(std::string_view a, std::string_view b) noexcept {
  if (a.length() == b.length()) {
    return std::memcmp(a.data(), b.data(), a.length());
  } else if (a.length() < b.length()) {
    int rc = std::memcmp(a.data(), b.data(), a.length());
    if (rc == 0)
      rc = -1;
    return rc;
  } else {
    int rc = std::memcmp(a.data(), b.data(), b.length());
    if (rc == 0)
      rc = 1;
    return rc;
  }
}

int compare_string_view_for_lt(std::string_view a,
                               std::string_view b) noexcept {
  if (a.length() >= b.length()) {
    return memcmp(a.data(), b.data(), b.length());
  } else {
    int rc = memcmp(a.data(), b.data(), a.length());
    if (rc == 0)
      rc = -1;
    return rc;
  }
}

size_t common_prefix_ex(const void* pa, const void* pb, size_t maxl,
                        bool utf8) noexcept {
  const char* a = static_cast<const char*>(pa);
  const char* b = static_cast<const char*>(pb);
  size_t k = 0;

#if defined __AVX2__ && defined __BMI__
  if (maxl >= 32) {
    __m256i ones = _mm256_set1_epi8(-1);
    __m256i va = *(const __m256i_u*)a;
    __m256i vb = *(const __m256i_u*)b;
    __m256i vc = _mm256_cmpeq_epi8(va, vb);
    if (_mm256_testc_si256(vc, ones)) {
      k = 32 - (uintptr_t(a) & 31);
      for (;;) {
        if (k >= maxl - 32) {
          va = *(const __m256i_u*)(a + maxl - 32);
          vb = *(const __m256i_u*)(b + maxl - 32);
          vc = _mm256_cmpeq_epi8(va, vb);
          k = maxl - 32 + _tzcnt_u32(~_mm256_movemask_epi8(vc));
          break;
        }
        va = *(const __m256i*)(a + k);
        vb = *(const __m256i_u*)(b + k);
        vc = _mm256_cmpeq_epi8(va, vb);
        if (!_mm256_testc_si256(vc, ones)) {
          k += ctz(~_mm256_movemask_epi8(vc));
          break;
        }
        k += 32;
      }
    } else {
      k = ctz(~_mm256_movemask_epi8(vc));
    }
  } else
#endif
  {
    while (k < maxl / 8 * 8 &&
           mempick<uint64_t>(a + k) == mempick<uint64_t>(b + k))
      k += 8;
    while ((k < maxl) && a[k] == b[k]) ++k;
  }

  if (utf8 && (k < maxl)) {
    while (k && utf8_byte_type(a[k]) == cbu::Utf8ByteType::TRAILING) --k;
  }

  return k;
}

size_t common_suffix_ex(const void* pa, const void* pb, size_t maxl,
                        bool utf8) noexcept {
  const char* a = static_cast<const char*>(pa);
  const char* b = static_cast<const char*>(pb);
  const char* aend = a;

  const char* min_a = a - maxl;
  ptrdiff_t d = b - a;

#if defined __AVX2__
  if (maxl >= 32) {
    __m256i ones = _mm256_set1_epi8(-1);
    a -= 32;
    __m256i va = *(const __m256i_u*)a;
    __m256i vb = *(const __m256i_u*)(a + d);
    __m256i vc = _mm256_cmpeq_epi8(va, vb);
    if (_mm256_testc_si256(vc, ones)) {
      uintptr_t misalign = -uintptr_t(a) & 31;
      a += misalign;
      for (;;) {
        if (a <= min_a + 32) {
          va = *(const __m256i_u*)min_a;
          vb = *(const __m256i_u*)(min_a + d);
          vc = _mm256_cmpeq_epi8(va, vb);
          a = min_a + 32 - _lzcnt_u32(~_mm256_movemask_epi8(vc));
          break;
        }
        a -= 32;
        va = *(const __m256i*)a;
        vb = *(const __m256i_u*)(a + d);
        vc = _mm256_cmpeq_epi8(va, vb);
        if (!_mm256_testc_si256(vc, ones)) {
          a += 32l - _lzcnt_u32(~_mm256_movemask_epi8(vc));
          break;
        }
      }
    } else {
      a += 32l - _lzcnt_u32(~_mm256_movemask_epi8(vc));
    }

  } else
#endif
  {
    while (a > min_a && a[-1] == (a + d)[-1]) --a;
  }

  if (utf8) {
    while (a < aend && int8_t(*a) < int8_t(0xc0)) ++a;
  }

  return aend - a;
}

}  // namespace cbu
