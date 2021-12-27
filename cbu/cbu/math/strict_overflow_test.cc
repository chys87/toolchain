
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

#include "cbu/math/strict_overflow.h"

#include <cmath>

#include <gtest/gtest.h>

namespace cbu {

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
