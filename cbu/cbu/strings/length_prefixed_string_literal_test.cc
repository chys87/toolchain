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

#include "cbu/strings/length_prefixed_string_literal.h"

#include <gtest/gtest.h>

#include "cbu/debug/gtest_formatters.h"

namespace cbu {
namespace {

using std::operator""sv;

TEST(LengthPrefixedStringLiteralTest, Default) {
  EXPECT_EQ("abc"_lpsl.view(), "abc"sv);
  EXPECT_EQ("abc"_lpsl16.view(), "abc"sv);

  EXPECT_EQ(u8"😊一二三abc"_lpsl.view(), u8"😊一二三abc"sv);
  EXPECT_EQ(u8"😊一二三abc"_lpsl16.view(), u8"😊一二三abc"sv);

  EXPECT_EQ(L"😊一二三abc"_lpsl.view(), L"😊一二三abc"sv);
  EXPECT_EQ(L"😊一二三abc"_lpsl16.view(), L"😊一二三abc"sv);

  EXPECT_EQ(u"😊一二三abc"_lpsl.view(), u"😊一二三abc"sv);
  EXPECT_EQ(u"😊一二三abc"_lpsl16.view(), u"😊一二三abc"sv);

  EXPECT_EQ(U"😊一二三abc"_lpsl.view(), U"😊一二三abc"sv);
  EXPECT_EQ(U"😊一二三abc"_lpsl16.view(), U"😊一二三abc"sv);
}

}  // namespace

}  // namespace cbu
