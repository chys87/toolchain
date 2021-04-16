/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021, chys <admin@CHYS.INFO>
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

#include "cbu/common/bitmask_ref.h"

#include <gtest/gtest.h>

namespace cbu {
namespace {

TEST(BitMaskRefTest, SingleBitOperationsTest) {
  uint32_t bits[2] = {};
  BitMaskRef r(bits);

  r.set(3);
  EXPECT_FALSE(r.test(0));
  EXPECT_FALSE(r.test(1));
  EXPECT_FALSE(r.test(2));
  EXPECT_TRUE(r.test(3));
  EXPECT_FALSE(r.test(4));

  r.reset(3);
  EXPECT_EQ(bits[0], 0);

  r.flip(3);
  EXPECT_FALSE(r.test(0));
  EXPECT_FALSE(r.test(1));
  EXPECT_FALSE(r.test(2));
  EXPECT_TRUE(r.test(3));
  EXPECT_FALSE(r.test(4));

  r.flip(3);
  EXPECT_EQ(bits[0], 0);
}

TEST(BitMaskRefTest, RangeOperationsTest) {
  constexpr unsigned kBits = 32 * 5;
  uint32_t bm[kBits / 32];
  BitMaskRef r(bm);

  for (unsigned lo = 0; lo < kBits; ++lo) {
    for (unsigned hi = lo; hi < kBits; ++hi) {
      memset(bm, 0, sizeof(bm));
      r.set(lo, hi);
      for (unsigned i = 0; i < kBits; ++i) {
        if (i >= lo && i < hi)
          ASSERT_TRUE(r.test(i)) << "lo=" << lo << ", hi=" << hi << ", i=" << i;
        else
          ASSERT_FALSE(r.test(i))
              << "lo=" << lo << ", hi=" << hi << ", i=" << i;
      }
    }
  }

  for (unsigned lo = 0; lo < kBits; ++lo) {
    for (unsigned hi = lo; hi < kBits; ++hi) {
      memset(bm, -1, sizeof(bm));
      r.reset(lo, hi);
      for (unsigned i = 0; i < kBits; ++i) {
        if (i >= lo && i < hi)
          ASSERT_FALSE(r.test(i))
              << "lo=" << lo << ", hi=" << hi << ", i=" << i;
        else
          ASSERT_TRUE(r.test(i)) << "lo=" << lo << ", hi=" << hi << ", i=" << i;
      }
    }
  }
}

}}  // namespace cbu
