/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

#include "fastarith.h"
#include <cmath>
#include <gtest/gtest.h>

namespace cbu {

TEST(FastPowTest, Unsigned) {
  EXPECT_EQ(1, fast_powu(2, 0));
  EXPECT_EQ(0x100000000ULL, fast_powu(2ULL, 32));
  EXPECT_EQ(0x400000000ULL, fast_powu(2ULL, 34));
  EXPECT_NEAR(2.0, fast_powu(10., 0, 2), 1e-10);
  EXPECT_NEAR(1e100, fast_powu(10., 100), 1e90);
}

TEST(FastPowTest, Signed) {
  EXPECT_NEAR(1e100, fast_powi(10., 100), 1e90);
  EXPECT_NEAR(1.0, fast_powi(10., 0), 1e-10);
  EXPECT_NEAR(1e-100, fast_powi(10., -100), 1e-110);
}

TEST(Math, Clamp) {
  EXPECT_TRUE(std::isnan(clamp(NAN, 1, 2)));
  EXPECT_FLOAT_EQ(2.f, clamp(1.f, 2, 3));
  EXPECT_FLOAT_EQ(3.f, clamp(4.f, 2, 3));
  EXPECT_FLOAT_EQ(2.5f, clamp(2.5f, 2, 3));
  EXPECT_FLOAT_EQ(1.f, clamp(1.f, NAN, 3));
  EXPECT_FLOAT_EQ(4.f, clamp(4.f, 2, NAN));
}

TEST(MapUToFloat, Float) {
  EXPECT_FLOAT_EQ(0.0f, map_uint32_to_float(0));
  EXPECT_FLOAT_EQ(0.5f, map_uint32_to_float(INT32_MAX));
  EXPECT_FLOAT_EQ(1.0f, map_uint32_to_float(UINT32_MAX));
  EXPECT_GT(1.0f, map_uint32_to_float(UINT32_MAX));
  EXPECT_LT(0.0f, map_uint32_to_float(1));
}

TEST(MapUToFloat, Double) {
  EXPECT_DOUBLE_EQ(0.0, map_uint64_to_double(0));
  EXPECT_DOUBLE_EQ(0.5, map_uint64_to_double(INT64_MAX));
  EXPECT_DOUBLE_EQ(1.0, map_uint64_to_double(UINT64_MAX));
  EXPECT_GT(1.0, map_uint64_to_double(UINT64_MAX));
  EXPECT_LT(0.0, map_uint64_to_double(1));
}

TEST(PowLog10, ILog10) {
  EXPECT_EQ(0, ilog10(0));
  EXPECT_EQ(0, ilog10(1));
  EXPECT_EQ(0, ilog10(2));
  EXPECT_EQ(0, ilog10(9));
  EXPECT_EQ(1, ilog10(10));
  EXPECT_EQ(1, ilog10(99));
  EXPECT_EQ(2, ilog10(100));
  EXPECT_EQ(2, ilog10(999));
  EXPECT_EQ(3, ilog10(1000));
  EXPECT_EQ(3, ilog10(9999));
  EXPECT_EQ(4, ilog10(10000));
  EXPECT_EQ(9, ilog10(4294967295));
}

} // namespace cbu
