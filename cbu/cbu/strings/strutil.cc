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

#include "cbu/strings/strutil.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <type_traits>
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#  include <x86intrin.h>
#endif
#ifdef __ARM_NEON
#  include <arm_neon.h>
#endif

#include "cbu/common/bit.h"
#include "cbu/common/byteorder.h"
#include "cbu/compat/compilers.h"
#include "cbu/math/common.h"
#include "cbu/math/strict_overflow.h"
#include "cbu/strings/encoding.h"
#include "cbu/strings/faststr.h"

namespace cbu {

using std::uint32_t;
using std::uint64_t;
using std::uintptr_t;
using std::intptr_t;
using std::size_t;
using ssize_t = std::make_signed_t<size_t>;

std::size_t strcnt(const char *s, char c) noexcept {
  if (c == 0) {
    return 0;
  }
  size_t r = 0;
#if defined __AVX2__ && !defined CBU_ADDRESS_SANITIZER
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
#if defined __AVX512BW__ && !defined CBU_ADDRESS_SANITIZER
  if (n == 0) return 0;

  __m512i ref = _mm512_set1_epi8(c);
  size_t misalign = (uintptr_t)s & 63;
  const __m512i *p = (const __m512i *)((uintptr_t)s & -64);

  uint64_t mask = _mm512_cmpeq_epi8_mask(ref, *p++);
  mask >>= misalign;

  size_t l = n + misalign - 64;
  if (ssize_t(l) <= 0) return _mm_popcnt_u64(_bzhi_u64(mask, n));

  r = _mm_popcnt_u64(mask);

  while (!sub_overflow(l, 64, &l))
    r += _mm_popcnt_u64(_mm512_cmpeq_epi8_mask(ref, *p++));

  unsigned tail = unsigned(l) & 63;
  if (tail)
    r += _mm_popcnt_u64(_bzhi_u64(_mm512_cmpeq_epi8_mask(ref, *p++), tail));

#elif defined __AVX2__ && defined __BMI2__ && !defined CBU_ADDRESS_SANITIZER
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
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ && !defined CBU_ADDRESS_SANITIZER
  if (n == 0) return 0;

  uint8x16_t ref = vdupq_n_u8(c);

  size_t misalign = (uintptr_t)s & 15;
  const uint8x16_t* p = (const uint8x16_t*)((uintptr_t)s & -16);

  uint64_t mask =
      vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(
                        vreinterpretq_u16_u8(uint8x16_t(ref == *p++)), 4)),
                    0);
  mask &= 0x1111'1111'1111'1111ull;
  mask >>= misalign * 4;

  size_t l = n + misalign;
  if (l <= 16) {
    mask &= (1ull << 4 << ((n - 1) * 4)) - 1;
    return uint32_t(std::popcount(mask));
  }
  l -= 16;

  r = uint32_t(std::popcount(mask));

  while (l >= 128) {
    l -= 128;
    uint8x16x4_t x = vld1q_u8_x4((const uint8_t*)p);
    uint8x16x4_t y = vld1q_u8_x4((const uint8_t*)p + 64);
    uint8x16_t va = uint8x16_t(ref == x.val[0]);
    uint8x16_t vb = uint8x16_t(ref == x.val[1]);
    uint8x16_t vc = uint8x16_t(ref == x.val[2]);
    uint8x16_t vd = uint8x16_t(ref == x.val[3]);
    uint8x16_t ve = uint8x16_t(ref == y.val[0]);
    uint8x16_t vf = uint8x16_t(ref == y.val[1]);
    uint8x16_t vg = uint8x16_t(ref == y.val[2]);
    uint8x16_t vh = uint8x16_t(ref == y.val[3]);
    uint8x16_t sum = va + vb + vc + vd + ve + vf + vg + vh;
    r -= int8_t(vaddvq_u8(sum));
    p += 8;
  }

  while (l >= 16) {
    l -= 16;
    uint8x16_t a = uint8x16_t(ref == *p++);
    uint8_t cnt = -vaddvq_u8(a);
    r += cnt;
  }

  if (l) {
    mask = vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(
                             vreinterpretq_u16_u8(uint8x16_t(ref == *p++)), 4)),
                         0);
    r += std::uint32_t(std::popcount(bzhi(mask, l* 4))) / 4;
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

#if defined __AVX2__ && !defined CBU_ADDRESS_SANITIZER
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

int strnumcmp(const char* a, size_t al, const char* b, size_t bl) noexcept {
  const uint8_t* u = reinterpret_cast<const uint8_t*>(a);
  const uint8_t* ue = u + al;
  const uint8_t* v = reinterpret_cast<const uint8_t*>(b);
  const uint8_t* ve = v + bl;
  unsigned U, V;

#if defined __AVX512BW__ && defined __AVX512VL__
  for (size_t xl = std::min(al, bl) / 16; xl; --xl) {
    __m128i uv = *(const __m128i_u*)u;
    __m128i vv = *(const __m128i_u*)v;
    uint16_t neqmask = _mm_cmpneq_epi8_mask(uv, vv);
    if (neqmask) {
      long offset = std::countr_zero(neqmask);
      u += offset;
      v += offset;
      break;
    }
    u += 16;
    v += 16;
  }
#elif defined __SSE4_1__
  for (size_t xl = std::min(al, bl) / 16; xl; --xl) {
    __m128i uv = *(const __m128i_u*)u;
    __m128i vv = *(const __m128i_u*)v;
    __m128i diff = uv ^ vv;
    if (!_mm_testz_si128(diff, diff)) {
      uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(diff, _mm_setzero_si128())) ^ 0xffff;
      long offset = __builtin_ctz(mask);
      u += offset;
      v += offset;
      break;
    }
    u += 16;
    v += 16;
  }
#elif defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  for (size_t xl = std::min(al, bl) / 16; xl; --xl) {
    uint8x8_t cmp =
        vshrn_n_u16(vreinterpretq_u16_u8(uint8x16_t(*(const uint8x16_t*)u !=
                                                    *(const uint8x16_t*)v)),
                    4);
    uint64_t mask = vget_lane_u64(vreinterpret_u64_u8(cmp), 0);
    if (mask != 0) {
      long offset = ctz(mask) / 4;
      u += offset;
      v += offset;
      break;
    }
    u += 16;
    v += 16;
  }
#endif
  for (;;) {
    if (u >= ue) {
      if (v >= ve) return 0;
      return -1;
    } else if (v >= ve) {
      return 1;
    }
    U = *u++;
    V = *v++;
    if (U != V) break;
  }

  int cmpresult = U - V;

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
    if (u >= ue) {
      if (v >= ve)
        return cmpresult;
      else if (*v >= '0' && *v <= '9')
        return -1;
      else
        return cmpresult;
    } else if (v >= ve) {
      if (*u >= '0' && *u <= '9')
        return 1;
      else
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
  if (p + 4 <= q) {
    uint16_t a = bswap(mempick2(p));
    uint16_t b = bswap(mempick2(q - 2));
    memdrop2(q - 2, a);
    memdrop2(p, b);
    p += 2;
    q -= 2;
  }
  if (p < --q) {
    std::swap(*p, *q);
  }
  return ret;
}

size_t common_prefix_ex(const void* pa, const void* pb, size_t maxl) noexcept {
  const char* a = static_cast<const char*>(pa);
  const char* b = static_cast<const char*>(pb);
  size_t k = 0;

#if defined __AVX512BW__
  for (;;) {
    if (k + 64 <= maxl) {
      __m512i va = *(const __m512i_u*)(a + k);
      __m512i vb = *(const __m512i_u*)(b + k);
      auto msk = _mm512_cmpneq_epi8_mask(va, vb);
      if (msk) {
        k += ctz(msk);
        break;
      }
      k += 64;
    } else if (k < maxl) {
      uint64_t msk = (uint64_t(1) << (maxl - k)) - 1;
      __m512i va = _mm512_maskz_loadu_epi8(msk, a + k);
      __m512i vb = _mm512_maskz_loadu_epi8(msk, b + k);
      msk = _mm512_cmpneq_epi8_mask(va, vb);
      k += ctz(msk);
      if (k >= maxl) return maxl;
      break;
    } else {
      return maxl;
    }
  }

#elif defined __AVX2__ && defined __BMI__
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
  } else {
    while (k != maxl / 8 * 8 &&
           mempick<uint64_t>(a + k) == mempick<uint64_t>(b + k))
      k += 8;
    while ((k != maxl) && a[k] == b[k]) ++k;
  }

#else
  // Clang can turn the main loop below into NEON, so we don't write a separate
  // case for NEON.  (Though NEON doesn't help much when we do 16 bytes a time)
  if (maxl >= 16) {
    bool found = false;
    for (k = 0; k != maxl / 16 * 16; k += 16) {
      uint64_t vaa = mempick_le<uint64_t>(a + k);
      uint64_t vab = mempick_le<uint64_t>(a + k + 8);
      uint64_t vba = mempick_le<uint64_t>(b + k);
      uint64_t vbb = mempick_le<uint64_t>(b + k + 8);
      uint64_t vca = vaa ^ vba;
      uint64_t vcb = vab ^ vbb;
      if ((vca | vcb) != 0) {
        if (vca != 0) {
          k += std::countr_zero(vca) / 8;
        } else {
          k += 8 + std::countr_zero(vcb) / 8;
        }
        found = true;
        break;
      }
    }

    if (!found) {
      uint64_t va = mempick_le<uint64_t>(a + maxl - 16);
      uint64_t vb = mempick_le<uint64_t>(b + maxl - 16);
      if (va != vb) {
        k = maxl - 16 + std::countr_zero(va ^ vb) / 8;
      } else {
        va = mempick_le<uint64_t>(a + maxl - 8);
        vb = mempick_le<uint64_t>(b + maxl - 8);
        k = maxl - 8 + std::countr_zero(va ^ vb) / 8;
      }
    }
  } else if (maxl >= 8) {
    uint64_t va = mempick_le<uint64_t>(a);
    uint64_t vb = mempick_le<uint64_t>(b);
    if (va != vb) {
      k = std::countr_zero(va ^ vb) / 8;
    } else {
      va = mempick_le<uint64_t>(a + maxl - 8);
      vb = mempick_le<uint64_t>(b + maxl - 8);
      k = maxl - 8 + std::countr_zero(va ^ vb) / 8;
    }
  } else {
    auto load = [maxl](const char* p) {
      uint64_t v = 0;
#ifdef CBU_ADDRESS_SANITIZER
      for (size_t i = 0; i < maxl; ++i) v |= uint64_t(uint8_t(p[i])) << (8 * i);
#else
      if ((uintptr_t(p) & 15) <= 8)
        v = mempick_le<uint64_t>(p);
      else
        v = mempick_le<uint64_t>(p + maxl - 8) >> (64 - maxl * 8);
#endif
      return v;
    };
    uint64_t va = load(a);
    uint64_t vb = load(b);
    k = std::countr_zero(va ^ vb) / 8;
    if (k > maxl) k = maxl;
  }
#endif

  return k;
}

size_t common_suffix_ex(const void* pa, const void* pb, size_t maxl) noexcept {
  const char* a = static_cast<const char*>(pa);
  const char* b = static_cast<const char*>(pb);
  const char* aend = a;

  const char* min_a = a - maxl;
  ptrdiff_t d = b - a;

#if defined __AVX512BW__
  for (;;) {
    if (a >= min_a + 64) {
      a -= 64;
      __m512i va = *(const __m512i_u*)a;
      __m512i vb = *(const __m512i_u*)(a + d);
      auto msk = _mm512_cmpneq_epi8_mask(va, vb);
      if (msk) {
        a += 64l - _lzcnt_u64(msk);
        break;
      }
    } else if (a > min_a) {
      uint64_t msk = ~(uint64_t(-1) >> (a - min_a));
      __m512i va = _mm512_maskz_loadu_epi8(msk, a - 64);
      __m512i vb = _mm512_maskz_loadu_epi8(msk, a - 64 + d);
      msk = _mm512_cmpneq_epi8_mask(va, vb);
      a -= _lzcnt_u64(msk);
      if (a <= min_a) a = min_a;
      break;
    } else {
      break;
    }
  }

  if (false)
#elif defined __AVX2__
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

  return aend - a;
}

size_t common_suffix_fix_for_utf8(const void* pa, size_t l) noexcept {
  if (l) {
    const char* a = static_cast<const char*>(pa);
    const char* p = a - l;
    while (p < a && int8_t(*p) < int8_t(0xc0)) ++p;
    l = a - p;
  }
  return l;
}

size_t truncate_string_utf8_impl(const void* s, size_t n) noexcept {
  const char8_t* p = static_cast<const char8_t*>(s);
  CBU_NAIVE_LOOP
  while (n && utf8_byte_type(p[n]) == Utf8ByteType::TRAILING) --n;
  return n;
}

size_t char_span_length(const void* buffer, size_t len, char c) noexcept {
  const char* p = static_cast<const char*>(buffer);
  size_t i = 0;

#ifdef __AVX512BW__
  __m512i ref = _mm512_set1_epi8(c);
  while (i + 64 <= len) {
    uint64_t neqmsk = _mm512_cmpneq_epi8_mask(
        ref, *reinterpret_cast<const __m512i_u*>(p + i));
    if (neqmsk) return i + _tzcnt_u64(neqmsk);
    i += 64;
  }
  if (i < len) {
    uint64_t neqmsk = _mm512_cmpneq_epi8_mask(
        ref, _mm512_maskz_loadu_epi8(_bzhi_u64(-1ull, len - i), p + i));
    i += _tzcnt_u64(neqmsk | (1ull << (len - i)));
  }
  return i;
#endif

#ifdef __AVX2__
  while (i + 32 <= len) {
    __m256i v =
        _mm256_set1_epi8(c) ^ *reinterpret_cast<const __m256i_u*>(p + i);
    if (_mm256_testz_si256(v, v)) {
      i += 32;
      continue;
    }
    uint32_t zmask =
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_setzero_si256()));
    return i + _tzcnt_u32(~zmask);
  }
#endif

  // The default implementation is good enough for aarch64.
  // NEON code which processed 16 bytes a time is not necessarily better.
  uint64_t ref_u64 = uint8_t(c) * 0x01010101'01010101ull;
  while (i + 16 <= len) {
    uint64_t va = mempick_le<uint64_t>(p + i) ^ ref_u64;
    uint64_t vb = mempick_le<uint64_t>(p + i + 8) ^ ref_u64;
    if ((va | vb) != 0) {
      i += va ? 0 : 8;
      va = va ? va : vb;
      return i + std::countr_zero(va) / 8u;
    }
    i += 16;
  }

  if (len >= 16) {
    uint64_t va = mempick_le<uint64_t>(p + len - 16) ^ ref_u64;
    uint64_t vb = mempick_le<uint64_t>(p + len - 8) ^ ref_u64;
    i = va ? len - 16 : len - 8;
    va = va ? va : vb;
    return i + std::countr_zero(va) / 8u;
  } else if (len >= 8) {
    uint64_t va = mempick_le<uint64_t>(p) ^ ref_u64;
    uint64_t vb = mempick_le<uint64_t>(p + len - 8) ^ ref_u64;
    i = va ? 0 : len - 8;
    va = va ? va : vb;
    return i + std::countr_zero(va) / 8u;
  } else {
    uint64_t v = 0;
#ifdef CBU_ADDRESS_SANITIZER
    for (size_t i = 0; i < len; ++i) v |= uint64_t(uint8_t(p[i])) << (8 * i);
#else
    if ((uintptr_t(p) & 15) <= 8)
      v = mempick_le<uint64_t>(p);
    else
      v = mempick_le<uint64_t>(p + len - 8) >> (64 - len * 8);
#endif
    v ^= ref_u64;
    i = std::countr_zero(v) / 8u;
    if (i > len) i = len;
    return i;
  }
}

}  // namespace cbu
