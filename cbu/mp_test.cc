/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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
  Word a_[128];
  size_t na_ = 0;
  Word b_[128];
  size_t nb_ = 0;
};

TEST_F(MpTest, Conversion) {
  EXPECT_EQ("123456789012345678901234567890", to_dec(a_, na_));
  EXPECT_EQ("18ee90ff6c373e0ee4e3f0ad2", to_hex(a_, na_));
  EXPECT_EQ("143564417755415637016711617605322", to_oct(a_, na_));
  EXPECT_EQ("11000111011101001000011111111011011000011011100111110000011101"
            "11001001110001111110000101011010010", to_bin(a_, na_));

  EXPECT_EQ("446371678903360124661747118626766461972311602250509962735",
            to_dec(b_, nb_));
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

TEST_F(MpTest, Compare) {
  EXPECT_GT(0, compare(a_, na_, b_, nb_));
  EXPECT_GT(0, compare(a_, na_, b_, na_));
  EXPECT_FALSE(eq(a_, na_, b_, nb_));
  EXPECT_FALSE(eq(a_, na_, b_, na_));
}

} // namespace mp
} // namespace cbu
