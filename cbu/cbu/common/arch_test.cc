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

#include "cbu/common/arch.h"

#include <sys/mman.h>

#include <gtest/gtest.h>

namespace cbu {
namespace {

#if defined __x86_64__ && defined __AVX2__
TEST(ArchTest, x86_in_range_epi8) {
  for (int lo = -128; lo <= 127; ++lo) {
    for (int hi = -128; hi <= 127; ++hi) {
      for (int x = -128; x <= 127; x += 32) {
        __m256i v = _mm256_add_epi8(
            _mm256_set1_epi8(x),
            _mm256_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                             15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                             28, 29, 30, 31));
        __m256i cmp = in_range_epi8(v, lo, hi);
        __v32qi qcmp = (__v32qi)cmp;
        for (int i = 0; i < 32; ++i) {
          bool got = qcmp[i];
          bool expected = (lo <= hi) ? (x + i >= lo && x + i <= hi)
                                     : (x + i >= lo || x + i <= hi);
          ASSERT_EQ(expected, got)
              << "lo=" << lo << " hi=" << hi << " x=" << x << " i=" << i;
        }
      }
    }
  }
}

// Make sure (a + 5 > 10) is not optimized as (a > 5)
TEST(ArchTest, x86_in_range_epi8_const) {
  __m128i v = _mm_set1_epi8(127);
  asm volatile("# %0" : "+x"(v));
  // v + 1 > 125 => false; But if it's implemented as v > 124, it'd be true
  v = in_range_epi8(v, 125, 126);
  EXPECT_EQ(0, v[0]);
}

TEST(ArchTest, load_unaligned_si256) {
  char* data = (char*)mmap(NULL, 4096 * 2, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, data);
  int rc = mprotect(data + 4096, 4096, PROT_NONE);
  ASSERT_EQ(0, rc);

  for (unsigned i = 0; i < 4096; ++i) data[i] = i;

  for (unsigned offset = 0; offset < 4096; ++offset) {
    for (unsigned bytes = 1; bytes <= 32 && offset + bytes <= 4096; ++bytes) {
      __m256i v = load_unaligned_si256(data + offset, bytes);
      char buf[32];
      _mm256_storeu_si256((__m256i*)buf, v);

      for (unsigned k = 0; k < bytes; ++k) ASSERT_EQ(char(offset + k), buf[k]);
    }
  }
}

TEST(ArchTest, byte_mask) {
  EXPECT_EQ('\xff', ((__v16qi)_mm_lobyte_mask(5))[4]);
  EXPECT_EQ('\x00', ((__v16qi)_mm_lobyte_mask(5))[5]);

  EXPECT_EQ('\x00', ((__v16qi)_mm_lobyte_negmask(5))[4]);
  EXPECT_EQ('\xff', ((__v16qi)_mm_lobyte_negmask(5))[5]);

  EXPECT_EQ('\xff', ((__v32qi)_mm256_lobyte_mask(25))[24]);
  EXPECT_EQ('\x00', ((__v32qi)_mm256_lobyte_mask(25))[25]);

  EXPECT_EQ('\x00', ((__v32qi)_mm256_lobyte_negmask(25))[24]);
  EXPECT_EQ('\xff', ((__v32qi)_mm256_lobyte_negmask(25))[25]);

  EXPECT_EQ(-1, ((__v8hi)_mm_lobyte_mask<2>(3))[2]);
  EXPECT_EQ(00, ((__v8hi)_mm_lobyte_mask<2>(3))[3]);
  EXPECT_EQ(-1, ((__v4si)_mm_lobyte_mask<4>(3))[2]);
  EXPECT_EQ(00, ((__v4si)_mm_lobyte_mask<4>(3))[3]);

  EXPECT_TRUE(isnan(_mm_lofloat_mask(3)[2]));
  EXPECT_EQ(0, _mm_lofloat_mask(3)[3]);

  EXPECT_EQ(0, _mm_lofloat_negmask(3)[2]);
  EXPECT_TRUE(isnan(_mm_lofloat_negmask(3)[3]));

  EXPECT_TRUE(isnan(_mm256_lofloat_mask(5)[4]));
  EXPECT_EQ(0, _mm256_lofloat_mask(5)[5]);

  EXPECT_EQ(0, _mm256_lofloat_negmask(5)[4]);
  EXPECT_TRUE(isnan(_mm256_lofloat_negmask(5)[5]));
}

TEST(ArchTest, load_unaligned_si128_str) {
  char* data = (char*)mmap(NULL, 4096 * 2, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, data);

  for (unsigned i = 0; i < 8192; ++i) data[i] = 'a' + (i % 16);
  data[8191] = '\0';

  for (unsigned i = 0; i < 4096 + 16; ++i) {
    __m128i v = load_unaligned_si128_str(&data[i]);
    char buf[16];
    _mm_storeu_si128((__m128i*)buf, v);
    for (unsigned j = 0; j < 16; ++j)
      ASSERT_EQ(char('a' + ((i + j) % 16)), buf[j]);
  }

  int rc = mprotect(data + 4096, 4096, PROT_NONE);
  ASSERT_EQ(0, rc);

  data[4095] = '\0';

  for (unsigned i = 4080; i < 4096; ++i) {
    __m128i v = load_unaligned_si128_str(&data[i]);
    char buf[16];
    _mm_storeu_si128((__m128i*)buf, v);
    for (unsigned j = i; j < 4095; ++j)
      ASSERT_EQ(char('a' + (j % 16)), buf[j - i]);
    ASSERT_EQ(0, buf[4095 - i]);
  }
}

TEST(ArchTest, load_unaligned_si128) {
  char* data = (char*)mmap(NULL, 4096 * 2, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, data);
  int rc = mprotect(data + 4096, 4096, PROT_NONE);
  ASSERT_EQ(0, rc);

  for (unsigned i = 0; i < 4096; ++i) data[i] = i;

  for (unsigned offset = 0; offset < 4096; ++offset) {
    for (unsigned bytes = 1; bytes <= 16 && offset + bytes <= 4096; ++bytes) {
      __m128i v = load_unaligned_si128(data + offset, bytes);
      char buf[16];
      _mm_storeu_si128((__m128i*)buf, v);

      for (unsigned k = 0; k < bytes; ++k) ASSERT_EQ(char(offset + k), buf[k]);
    }
  }
}

#endif

}  // namespace
}  // namespace cbu
