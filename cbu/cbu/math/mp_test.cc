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

#include "mp.h"
#include <gtest/gtest.h>

namespace cbu {
namespace mp {

class MpTest : public testing::Test {
 public:
  void SetUp() override {
    na_ = from_dec(a_, "123456789012345678901234567890", 30);
    nb_ = from_hex(b_, "1234567890abcdef1234567890abcdef1234567890abcdef", 48);
  }

 protected:
  Word a_[128] = {};
  size_t na_ = 0;
  Word b_[128] = {};
  size_t nb_ = 0;
};

TEST_F(MpTest, ConversionToString) {
  EXPECT_EQ("123456789012345678901234567890", to_dec(a_, na_));
  EXPECT_EQ("18ee90ff6c373e0ee4e3f0ad2", to_hex(a_, na_));
  EXPECT_EQ("143564417755415637016711617605322", to_oct(a_, na_));
  EXPECT_EQ("11000111011101001000011111111011011000011011100111110000011101"
            "11001001110001111110000101011010010", to_bin(a_, na_));

  EXPECT_EQ("446371678903360124661747118626766461972311602250509962735",
            to_dec(b_, nb_));
}

TEST_F(MpTest, ConversionFromString) {
  std::string BIN = "11011010111101011000110001011001111111010110000010110010101101011111101011110100011110010111101001010110100110000100000010110111000011111000010100110111001111011000100011101100011101000111101010110001101110101101101000010000011000010011101000000010011000011011100010110001111010010011001000010101010011110101100001011100101110100000001101001110101011101110101011001111111000000111010111110100110110000011111011010110001111101100111101111111111001100110110000111111110100110000111001000010111110000100000011101111";
  std::string OCT = "332753061317726026255375364362751264604026703702467173043543507526156555020302350023033426172231025236541345640151653565317700727646603732617547577714660776460710276040357";
  std::string DEC = "11467822398418588504477755208157500410311175314974656722676929816130726283649650936307216806217640809655882045847936824614123136682774030491049927296696559";
  std::string HEX = "daf58c59fd60b2b5faf4797a569840b70f85373d88ec747ab1bada10613a0261b8b1e932154f585cba034eaeeacfe075f4d83ed63ecf7fe66c3fd30e42f840ef";

  {
    Word buf[128];
    Ref a(buf, Radixed<Radix::BIN>{BIN});
    EXPECT_EQ(a.to_str<Radix::BIN>(), BIN) << "BIN -> BIN";
    EXPECT_EQ(a.to_str<Radix::DEC>(), DEC) << "BIN -> DEC";
    EXPECT_EQ(a.to_str<Radix::OCT>(), OCT) << "BIN -> OCT";
    EXPECT_EQ(a.to_str<Radix::HEX>(), HEX) << "BIN -> HEX";
  }
  {
    Word buf[128];
    Ref a(buf, Radixed<Radix::OCT>{OCT});
    EXPECT_EQ(a.to_str<Radix::BIN>(), BIN) << "OCT -> BIN";
    EXPECT_EQ(a.to_str<Radix::DEC>(), DEC) << "OCT -> DEC";
    EXPECT_EQ(a.to_str<Radix::OCT>(), OCT) << "OCT -> OCT";
    EXPECT_EQ(a.to_str<Radix::HEX>(), HEX) << "OCT -> HEX";
  }
  {
    Word buf[128];
    Ref a(buf, Radixed<Radix::DEC>{DEC});
    EXPECT_EQ(a.to_str<Radix::BIN>(), BIN) << "DEC -> BIN";
    EXPECT_EQ(a.to_str<Radix::DEC>(), DEC) << "DEC -> DEC";
    EXPECT_EQ(a.to_str<Radix::OCT>(), OCT) << "DEC -> OCT";
    EXPECT_EQ(a.to_str<Radix::HEX>(), HEX) << "DEC -> HEX";
  }
  {
    Word buf[128];
    Ref a(buf, Radixed<Radix::HEX>{HEX});
    EXPECT_EQ(a.to_str<Radix::BIN>(), BIN) << "HEX -> BIN";
    EXPECT_EQ(a.to_str<Radix::DEC>(), DEC) << "HEX -> DEC";
    EXPECT_EQ(a.to_str<Radix::OCT>(), OCT) << "HEX -> OCT";
    EXPECT_EQ(a.to_str<Radix::HEX>(), HEX) << "HEX -> HEX";
  }
}

TEST_F(MpTest, Add) {
  Word c[128];
  size_t nc = add(c, a_, na_, b_, nb_);
  EXPECT_EQ("446371678903360124661747118750223250984657281151744530625",
            to_dec(c, nc));

  nc = add(c, c, nc, 65536);
  EXPECT_EQ("446371678903360124661747118750223250984657281151744596161",
            to_dec(c, nc));
}

TEST_F(MpTest, Mul) {
  Word c[128];
  size_t nc = mul(c, b_, nb_, 123456789);
  EXPECT_EQ("55107614177947882301379010395662676847992197321293133611772757915",
            to_dec(c, nc));

  nc = mul(c, a_, na_, b_, nb_);
  EXPECT_EQ("1c5df134c1e90a3c9f4c019523403602821745a32340360265b9546e"
            "61572bc5e2cb440e",
            to_hex(c, nc));
}

TEST_F(MpTest, Div) {
  Word c[128];
  auto [nc, remainder] = div(c, b_, nb_, 99999);
  EXPECT_EQ("4463761426647867725294724133508999709720213224637346",
            to_dec(c, nc));
  EXPECT_EQ(81, remainder);
}

TEST_F(MpTest, RefTest) {
  Ref a(a_, na_ + 5);
  MinRef a_minimized(a);
  EXPECT_EQ(na_, a_minimized.size());

  static_assert(std::is_constructible_v<ConstRef, Ref>);
  static_assert(!std::is_constructible_v<Ref, ConstRef>);
  static_assert(std::is_constructible_v<Ref, MinRef>);
  static_assert(std::is_constructible_v<MinRef, MinRef>);
  static_assert(std::is_constructible_v<MinRef, Ref>);
}

TEST_F(MpTest, Compare) {
  EXPECT_GT(0, compare(a_, na_, b_, nb_));
  EXPECT_GT(0, compare(a_, na_, b_, na_));
  EXPECT_FALSE(eq(a_, na_, b_, nb_));
  EXPECT_FALSE(eq(a_, na_, b_, na_));
}

} // namespace mp
} // namespace cbu
