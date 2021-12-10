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

#include "cbu/common/str_to_integer.h"

#include <gtest/gtest.h>

namespace cbu {

TEST(StrToIntegerTest, Regular) {
  EXPECT_EQ((str_to_integer<int32_t>("123").value_or(0)), 123);
  EXPECT_EQ((str_to_integer<int32_t, RadixTag<8>>("123").value_or(0)), 0123);
  EXPECT_EQ((str_to_integer<int32_t, RadixTag<16>>("123").value_or(0)), 0x123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("123").value_or(0)), 123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0123").value_or(0)), 0123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0o123").value_or(0)), 0123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0O123").value_or(0)), 0123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0x123").value_or(0)), 0x123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0X123").value_or(0)), 0x123);
  EXPECT_EQ((str_to_integer<int32_t, AutoRadixTag>("0XAbCdEf").value_or(0)),
            0xabcdef);
  EXPECT_EQ((str_to_integer<int32_t>("-3").value_or(0)), -3);
  EXPECT_EQ((str_to_integer<int32_t>("+3").value_or(0)), 3);

  EXPECT_EQ((str_to_integer<int, RadixTag<36>>("-zz")).value_or(0),
            -(35 * 36 + 35));
}

TEST(StrToIntegerTest, InvalidFormat) {
  EXPECT_FALSE(str_to_integer<int32_t>(""));
  EXPECT_FALSE(str_to_integer<int32_t>("A"));
  EXPECT_FALSE(str_to_integer<int32_t>("--3"));
  EXPECT_FALSE(str_to_integer<int32_t>("++3"));
  EXPECT_FALSE(str_to_integer<int32_t>("+-3"));
  EXPECT_FALSE(str_to_integer<int32_t>("-+3"));
  EXPECT_FALSE(str_to_integer<int32_t>("0x5"));
  EXPECT_FALSE((str_to_integer<int32_t, AutoRadixTag>("08")));
}

TEST(StrToIntegerTest, Overflow) {
  // Signed
  EXPECT_TRUE(str_to_integer<int32_t>("2147483647").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("2147483648").has_value());
  EXPECT_TRUE(str_to_integer<int32_t>("-2147483647").has_value());
  EXPECT_TRUE(str_to_integer<int32_t>("-2147483648").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("-2147483649").has_value());

  EXPECT_FALSE(str_to_integer<int32_t>("4294967295").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("4294967296").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("-4294967295").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("-4294967296").has_value());
  EXPECT_FALSE(str_to_integer<int32_t>("-4294967297").has_value());

  // Unsinged
  EXPECT_TRUE(str_to_integer<uint32_t>("4294967295").has_value());
  EXPECT_FALSE(str_to_integer<uint32_t>("4294967296").has_value());

  // Smaller than int32
  EXPECT_TRUE(str_to_integer<int8_t>("127").has_value());
  EXPECT_FALSE(str_to_integer<int8_t>("128").has_value());
  EXPECT_TRUE(str_to_integer<uint8_t>("128").has_value());
  EXPECT_TRUE(str_to_integer<uint8_t>("255").has_value());
  EXPECT_FALSE(str_to_integer<uint8_t>("256").has_value());

  // Custom threshold
  EXPECT_EQ((str_to_integer<int, OverflowThresholdTag<123>>("123").value_or(0)),
            123);
  EXPECT_EQ((str_to_integer<int, OverflowThresholdTag<123>>("124").value_or(0)),
            0);
}

TEST(StrToIntegerTest, Partial) {
  EXPECT_EQ(str_to_integer_partial<int64_t>("1 + 2 = 3").value_opt.value_or(0),
            1);
}

TEST(StrToIntegerTest, Constexpr) {
  constexpr auto a = str_to_integer<int32_t>("2147483647");
  EXPECT_EQ(a.value_or(0), 2147483647);
  constexpr auto b = str_to_integer<int32_t>("2147483648");
  EXPECT_EQ(b.value_or(0), 0);
  constexpr auto c = str_to_integer<int, AutoRadixTag>("0xabcdef");
  EXPECT_EQ(c.value_or(0), 0xabcdef);
  constexpr auto d = str_to_integer<int, AutoRadixTag>("0o1234567");
  EXPECT_EQ(d.value_or(0), 01234567);
}

TEST(StrToIntegerTest, OverflowTorture) {
  uint64_t x = 0;
  do {
    {
      std::string s = std::to_string(x);

      ASSERT_EQ(str_to_integer<uint64_t>(s).value_or(x + 1), x) << x;

      ASSERT_EQ(
          (str_to_integer<uint64_t, cbu::OverflowThresholdTag<12345678>>(s))
              .value_or(x + 1),
          (x <= 12345678) ? x : x + 1)
          << x;

      ASSERT_EQ(
          (str_to_integer<uint64_t,
                          cbu::OverflowThresholdTag<0xffff'ffff'ffff'fff9>>(s))
              .value_or(x + 1),
          (x <= 0xffff'ffff'ffff'fff9) ? x : x + 1)
          << x;

      ASSERT_EQ(str_to_integer<int64_t>(s).value_or(-1),
                int64_t(x) >= 0 ? x : -1)
          << x;

      ASSERT_EQ(
          (str_to_integer<int64_t, cbu::OverflowThresholdTag<1234567890>>(s))
              .value_or(-1),
          (x <= 1234567890) ? int64_t(x) : -1);

      ASSERT_EQ(
          (str_to_integer<int64_t,
                          cbu::OverflowThresholdTag<0x7fff'ffff'ffff'fff9>>(s))
              .value_or(-1),
          x <= 0x7fff'ffff'ffff'fff9 ? int64_t(x) : -1)
          << x;

      ASSERT_EQ(str_to_integer<uint32_t>(s).value_or(2554),
                uint32_t(x) == x ? x : 2554)
          << x;
      ASSERT_EQ(str_to_integer<int32_t>(s).value_or(-1),
                x <= 0x7fffffff ? int32_t(x) : -1)
          << x;

      ASSERT_EQ(str_to_integer<uint16_t>(s).value_or(2554),
                uint16_t(x) == x ? x : 2554)
          << x;
      ASSERT_EQ(str_to_integer<int16_t>(s).value_or(-1),
                x <= 0x7fff ? int16_t(x) : -1)
          << x;
    }

    {
      int64_t y = x;
      std::string s = std::to_string(y);

      ASSERT_EQ(str_to_integer<int64_t>(s).value_or(y + 1), y) << y;
      ASSERT_EQ(str_to_integer<uint64_t>(s).value_or(x + 1),
                (y >= 0) ? y : x + 1)
          << y;

      ASSERT_EQ(str_to_integer<int32_t>(s).value_or(int32_t(y) + 1),
                int32_t(y) == y ? y : int32_t(y) + 1)
          << y;
      ASSERT_EQ(str_to_integer<int8_t>(s).value_or(int8_t(y) + 1),
                int8_t(y) == y ? y : int8_t(y) + 1)
          << y;
    }

    uint64_t new_x = x | (x >> 1);
    if (new_x != x)
      x = new_x;
    else
      ++x;
  } while (x != 0);

}

}  // namespace cbu
