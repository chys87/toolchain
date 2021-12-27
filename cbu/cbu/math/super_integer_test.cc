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

#include "cbu/math/super_integer.h"

#include <cmath>

#include <gtest/gtest.h>

namespace cbu {

TEST(SuperIntegerTest, ConversionAndCompare) {
  SuperInteger a(5);
  EXPECT_TRUE(a.pos);
  EXPECT_EQ(a.abs, 5);

  SuperInteger b(std::numeric_limits<int64_t>::min());
  EXPECT_FALSE(b.pos);
  EXPECT_EQ(b.abs, -uint64_t(std::numeric_limits<int64_t>::min()));

  EXPECT_GT(a, b);
  EXPECT_GE(a, b);
  EXPECT_LE(b, a);
  EXPECT_LT(b, a);

  EXPECT_GT(SuperInteger(12345), SuperInteger(1));
  EXPECT_LT(SuperInteger(-12345), SuperInteger(-1));

  EXPECT_TRUE(SuperInteger<uint8_t>(uint8_t(255)).fits_in<uint8_t>());
  EXPECT_FALSE(SuperInteger<uint8_t>(uint8_t(255)).fits_in<int8_t>());
  EXPECT_TRUE(SuperInteger<uint8_t>(int8_t(-1)).fits_in<int8_t>());
  EXPECT_FALSE(SuperInteger<uint8_t>(int8_t(-1)).fits_in<uint8_t>());
  EXPECT_FALSE((-SuperInteger<uint8_t>(uint8_t(255))).fits_in<uint8_t>());
  EXPECT_FALSE((-SuperInteger<uint8_t>(uint8_t(255))).fits_in<int8_t>());
}

TEST(SuperIntegerTest, AddOverflow) {
  using S = SuperInteger<uint8_t>;

  {
    S a(uint8_t(255));
    ASSERT_FALSE(a.add_overflow(uint8_t(0)));
    ASSERT_EQ(a, uint8_t(255));

    ASSERT_FALSE(a.add_overflow(int8_t(-1)));
    ASSERT_EQ(a, uint8_t(254));

    ASSERT_TRUE(a.add_overflow(int8_t(3)));
    ASSERT_EQ(a, uint8_t(1));
  }
  {
    S a = -S(uint8_t(255));
    ASSERT_FALSE(a.add_overflow(uint8_t(1)));
    ASSERT_EQ(a.abs, 254);

    ASSERT_TRUE(a.add_overflow(int8_t(-3)));
    ASSERT_FALSE(a.pos);
    ASSERT_EQ(a.abs, 1);
  }
}

TEST(SuperIntegerTest, MulOverflow) {
  using S = SuperInteger<uint8_t>;
  EXPECT_FALSE(S(uint8_t(255)).mul_overflow(int8_t(0)));
  EXPECT_FALSE(S(uint8_t(255)).mul_overflow(int8_t(1)));
  EXPECT_FALSE(S(uint8_t(255)).mul_overflow(int8_t(-1)));
  EXPECT_TRUE(S(uint8_t(255)).mul_overflow(int8_t(-2)));
  EXPECT_TRUE(S(uint8_t(255)).mul_overflow(int8_t(2)));
}

}// namespace cbu
