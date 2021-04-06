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

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>

namespace cbu {

// We define it so that gtests prints more readable messages, but we don't
// define it in the header to avoid including <iostream>
template <typename T>
std::ostream& operator<<(std::ostream& s, const SuperInteger<T>& v) {
  if (!v.pos) s << '-';
  return s << std::conditional_t<sizeof(T) == 1, unsigned int, T>(v.abs);
}

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

  EXPECT_EQ(0, ilog10_impl(0));
  EXPECT_EQ(0, ilog10_impl(1));
  EXPECT_EQ(0, ilog10_impl(2));
  EXPECT_EQ(0, ilog10_impl(9));
  EXPECT_EQ(1, ilog10_impl(10));
  EXPECT_EQ(1, ilog10_impl(99));
  EXPECT_EQ(2, ilog10_impl(100));
  EXPECT_EQ(2, ilog10_impl(999));
  EXPECT_EQ(3, ilog10_impl(1000));
  EXPECT_EQ(3, ilog10_impl(9999));
  EXPECT_EQ(4, ilog10_impl(10000));
  EXPECT_EQ(9, ilog10_impl(4294967295));
}

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

template <typename V, typename T, typename U>
inline consteval std::pair<bool, V> add_wrap(T a, U b) noexcept {
  V res;
  bool overflow = add_overflow(a, b, &res);
  return {overflow, res};
}

TEST(OverflowTest, ConstexprAddOverflow) {
  {
    constexpr auto a = add_wrap<uint32_t>(255, 1);
    ASSERT_FALSE(a.first);
    EXPECT_EQ(a.second, 256);
  }
  {
    constexpr auto a = add_wrap<uint8_t>(255, 1);
    ASSERT_TRUE(a.first);
    EXPECT_EQ(a.second, 0);
  }
  {
    constexpr auto a = add_wrap<uint8_t>(254, 1);
    ASSERT_FALSE(a.first);
    EXPECT_EQ(a.second, 255);
  }
  {
    constexpr auto a = add_wrap<int8_t>(254, 1);
    ASSERT_TRUE(a.first);
    EXPECT_EQ(a.second, -1);
  }
}

template <typename V, typename T, typename U>
inline consteval std::pair<bool, V> mul_wrap(T a, U b) noexcept {
  V res;
  bool overflow = mul_overflow(a, b, &res);
  return {overflow, res};
}

TEST(OverflowTest, ConstexprMulOverflow) {
  {
    constexpr auto a = mul_wrap<uint32_t>(128, 128);
    ASSERT_FALSE(a.first);
    ASSERT_EQ(a.second, 16384);
  }
  {
    constexpr auto a = mul_wrap<uint8_t>(127, 2);
    ASSERT_FALSE(a.first);
    ASSERT_EQ(a.second, 254);
  }
  {
    constexpr auto a = mul_wrap<int8_t>(127, 2);
    ASSERT_TRUE(a.first);
    ASSERT_EQ(a.second, -2);
  }
  {
    constexpr auto a = mul_wrap<int8_t>(-64, 2);
    ASSERT_FALSE(a.first);
    ASSERT_EQ(a.second, -128);
  }
  {
    constexpr auto a = mul_wrap<int8_t>(64, 2);
    ASSERT_TRUE(a.first);
    ASSERT_EQ(a.second, -128);
  }
}

}  // namespace cbu
