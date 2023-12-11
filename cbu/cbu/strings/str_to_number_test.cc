/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include <math.h>

#ifdef __SSE__
#  include <x86intrin.h>
#endif

#include <gtest/gtest.h>

#include "cbu/common/defer.h"
#include "cbu/strings/str_to_number.h"

namespace cbu {

TEST(StrToIntegerTest, Regular) {
  EXPECT_EQ((str_to_integer<int32_t>("123").value_or(0)), 123);
  EXPECT_EQ((str_to_integer<int32_t, RadixTag<8>>("123").value_or(0)), 0123);
  EXPECT_EQ((str_to_integer<int32_t, RadixTag<16>>("123").value_or(0)), 0x123);
  EXPECT_EQ((str_to_integer<int32_t, RadixTag<16>>("00391").value_or(0)), 0x391);
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

  // Tests that we don't get trapped in the octal path and results in an error
  EXPECT_EQ((str_to_integer_partial<int64_t, AutoRadixTag>("0 = 0"))
                .value_opt.value_or(-1),
            0);
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
TEST(StrToFpTest, Regular) {
  EXPECT_DOUBLE_EQ(str_to_fp<double>("3.14").value_or(NAN), 3.14);
  EXPECT_DOUBLE_EQ(str_to_fp<double>("-3.14").value_or(NAN), -3.14);
  EXPECT_DOUBLE_EQ(str_to_fp<double>("3.14e100").value_or(NAN), 3.14e100);
  EXPECT_DOUBLE_EQ(str_to_fp<double>("3.14e-100").value_or(NAN), 3.14e-100);
  EXPECT_DOUBLE_EQ(str_to_fp<double>(".5").value_or(NAN), .5);
  EXPECT_DOUBLE_EQ(str_to_fp<double>("5.").value_or(NAN), 5.);
  EXPECT_FALSE(str_to_fp<double>(".").has_value());
  EXPECT_FALSE(str_to_fp<float>("1.2e+x").has_value());
  EXPECT_DOUBLE_EQ(
      (str_to_fp<double, AutoRadixTag>("0x1.abcp-7").value_or(NAN)),
      0x1.abcp-7);
  EXPECT_DOUBLE_EQ((str_to_fp<double, AutoRadixTag>("0x1.abcp7").value_or(NAN)),
                   0x1.abcp7);
  EXPECT_DOUBLE_EQ(
      (str_to_fp<double, AutoRadixTag>("-0x1.abcp7").value_or(NAN)),
      -0x1.abcp7);
  EXPECT_DOUBLE_EQ((str_to_fp<double, AutoRadixTag>("-0x1.p7").value_or(NAN)),
                   -0x1.p7);
  EXPECT_DOUBLE_EQ((str_to_fp<double, AutoRadixTag>("-0x.5p7").value_or(NAN)),
                   -0x.5p7);
  EXPECT_DOUBLE_EQ((str_to_fp<double, AutoRadixTag>("-0x.5p-7").value_or(NAN)),
                   -0x.5p-7);
  EXPECT_TRUE(isnan(str_to_fp<double, AutoRadixTag>("-0x2.c").value_or(NAN)));
  EXPECT_TRUE(isnan(str_to_fp<double, AutoRadixTag>("-0x.p7").value_or(NAN)));

  double pos_zero = str_to_fp<double>("0").value_or(NAN);
  EXPECT_DOUBLE_EQ(pos_zero, 0.0);
  EXPECT_FALSE(signbit(pos_zero));
  double neg_zero = str_to_fp<double>("-0").value_or(NAN);
  EXPECT_DOUBLE_EQ(neg_zero, 0.0);
  EXPECT_TRUE(signbit(neg_zero));
  EXPECT_DOUBLE_EQ(INFINITY, str_to_fp<double>("inf").value_or(NAN));
  EXPECT_DOUBLE_EQ(INFINITY, str_to_fp<double>("Inf").value_or(NAN));
  EXPECT_DOUBLE_EQ(-INFINITY, str_to_fp<double>("-Inf").value_or(NAN));
  EXPECT_TRUE(isnan(str_to_fp<double>("nan").value_or(0)));
  EXPECT_TRUE(isnan(str_to_fp<double>("-nan").value_or(0)));

  static const double floats[]{
      -1.e10,  -1.e20,   0.,      5.1e-5,
      2.5e-2,  5.4,      8.13e2,  14839293847889234,
      FLT_MAX, -FLT_MAX, DBL_MAX, -DBL_MAX,
  };
  for (double x : floats) {
    char buf[1024];

    snprintf(buf, sizeof(buf), "%.20f", x);
    EXPECT_DOUBLE_EQ(x, (str_to_fp<double, AutoRadixTag>(buf).value_or(NAN)))
        << buf;
    EXPECT_FLOAT_EQ(float(x),
                    (str_to_fp<float, AutoRadixTag>(buf).value_or(NAN)))
        << buf;

    snprintf(buf, sizeof(buf), "%.20e", x);
    EXPECT_DOUBLE_EQ(x, (str_to_fp<double, AutoRadixTag>(buf).value_or(NAN)))
        << buf;
    EXPECT_FLOAT_EQ(float(x),
                    (str_to_fp<float, AutoRadixTag>(buf).value_or(NAN)))
        << buf;

    snprintf(buf, sizeof(buf), "%.20a", x);
    EXPECT_DOUBLE_EQ(x, (str_to_fp<double, AutoRadixTag>(buf).value_or(NAN)))
        << buf;
    EXPECT_FLOAT_EQ(float(x),
                    (str_to_fp<float, AutoRadixTag>(buf).value_or(NAN)))
        << buf;
  }
}

TEST(StrToFpTest, CornerCases) {
  // Test for min and max values
  for (int i = 0; i < 2; ++i) {
#ifdef __x86_64__
    unsigned int old_mxcsr = _mm_getcsr();
    // 0x8000: Flush to zero
    // 0x40: Denormal as zero
    unsigned int new_mxcsr =
        (i == 0) ? old_mxcsr & ~(0x8000 | 0x40) : old_mxcsr | (0x8000 | 0x40);
    _mm_setcsr(new_mxcsr);
    CBU_DEFER { _mm_setcsr(old_mxcsr); };

    SCOPED_TRACE((i == 0 ? "Default mode" : "FTZ & DAZ"));
#endif

    // Test for intermediate overflow
    // "DMIN" means denormal_min
    static constexpr std::pair<float, std::string_view> kFloats[]{
        // FLT_MAX   3.4028234663852885981e+38   0x1.fffffe00000000000000p+127
        {3.4028234663852885981e+38f, "3.4028234663852885981e+38"},
        {3.4028234663852885981e+38f,
         "0.000000000000000000000000034028234663852885981e+64"},
        {0x1.fffffe00000000000000p+127f,
         "0x1.fffffe00000000000000000000000123456789abcdefp+127"},
        // FLT_MIN   1.175494350822287508e-38    0x1.00000000000000000000p-126
        {0x1p-126f, "0x1p-126"},
        {0x1p-126f, "0x2p-127"},
        {0x1p-126f, "0x4p-128"},
        // FLT_DMIN  1.4012984643248170709e-45   0x1.00000000000000000000p-149
        {0x1p-149f, "0x1p-149"},
        {0x1p-149f, "0x11p-153"},
        {0x1p-149f, "0x8ffffffffffffffffffffffffffp-256"},
    };
    for (auto [flt, s] : kFloats) {
      // In DAZ mode, ignore denormal results
      if (flt * 2 > 0) {
        auto v = str_to_fp<float, AutoRadixTag>(s).value_or(NAN);
        EXPECT_FLOAT_EQ(v, flt) << s;
        EXPECT_GT(fabsf(v), 0) << s;
      }
    }

    static constexpr std::pair<double, std::string_view> kDoubles[]{
        // DBL_MAX   1.7976931348623157081e+308  0x1.fffffffffffff0000000p+1023
        {1.7976931348623157081e+308, "1.7976931348623157081e+308"},
        {0x1.fffffffffffff0000000p+1023, "0x0.fffffffffffff1p+1024"},
        // DBL_MIN   2.2250738585072013831e-308  0x1.00000000000000000000p-1022
        {0x1p-1022, "0x1p-1022"},
        {0x1p-1022, "0x4p-1024"},
        // DBL_DMIN  4.9406564584124654418e-324  0x1.00000000000000000000p-1074
        {0x1p-1074, "0x1p-1074"},
    };
    for (auto [dbl, s] : kDoubles) {
      // In DAZ mode, ignore denormal results
      if (dbl * 2 > 0) {
        auto v = str_to_fp<double, AutoRadixTag>(s).value_or(NAN);
        EXPECT_DOUBLE_EQ(v, dbl) << s;
        EXPECT_GT(fabs(v), 0) << s;
      }
    }
  }

  // Test for other special values
  {
    static constexpr std::tuple<float, double, std::string_view> kFps[]{
        // 0
        {0.f, 0., "0e-19999999"},
        {-0.f, -0., "-0e-19999999"},
        {0.f, 0., "0x0.0p-12345678"},
        {-0.f, -0., "-0x0.0p-12345678"},
        {0.f, 0., "0e19999999"},
        {-0.f, -0., "-0e19999999"},
        {0.f, 0., "0x0.0p12345678"},
        {-0.f, -0., "-0x0.0p12345678"},
        // very large exponents
        {0.f, 0., "1e-11111111"},
        {-0.f, -0., "-1e-11111111"},
        {0.f, 0., "0x1p-11111111"},
        {-0.f, -0., "-0x1p-11111111"},
        {INFINITY, INFINITY, "1e1111111"},
        {-INFINITY, -INFINITY, "-1e1111111"},
        {INFINITY, INFINITY, "0x1p1111111"},
        {-INFINITY, -INFINITY, "-0x1p1111111"},
        // overflow exponents
        {0.f, 0., "1e-4294967296"},
        {-0.f, -0., "-1e-4294967296"},
        {0.f, 0., "0x1p-4294967296"},
        {-0.f, -0., "-0x1p-4294967296"},
        {INFINITY, INFINITY, "1e4294967296"},
        {-INFINITY, -INFINITY, "-1e4294967296"},
        {INFINITY, INFINITY, "0x1p+4294967296"},
        {-INFINITY, -INFINITY, "-0x1p+4294967296"},
    };
    for (auto [flt, dbl, s] : kFps) {
      auto cf = str_to_fp<float, AutoRadixTag>(s).value_or(NAN);
      EXPECT_FLOAT_EQ(cf, flt) << s;
      auto cd = str_to_fp<double, AutoRadixTag>(s).value_or(NAN);
      EXPECT_DOUBLE_EQ(cd, dbl) << s;
    }
  }
}

TEST(StrToFpTest, Constexpr) {
  constexpr auto a = str_to_fp<float>("3.14159");
  EXPECT_FLOAT_EQ(a.value_or(0), 3.14159f);
  constexpr auto b = str_to_fp<double>("-100e10");
  EXPECT_DOUBLE_EQ(b.value_or(0), -100e10);
  constexpr auto c = str_to_fp<float, AutoRadixTag>("0xabcdefp+2");
  EXPECT_FLOAT_EQ(c.value_or(0), 0xabcdefp+2f);
  constexpr auto d = str_to_fp<double, AutoRadixTag>("0x.abcdefp-1000");
  EXPECT_DOUBLE_EQ(d.value_or(0), 0x.abcdefp-1000);
}

}  // namespace cbu
