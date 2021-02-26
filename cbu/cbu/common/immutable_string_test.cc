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

#include "immutable_string.h"
#include <gtest/gtest.h>

namespace cbu {

using namespace std::literals;

TEST(ImmutableStringTest, Construction) {

  EXPECT_TRUE(ImmutableString("x").holds_copy());
  EXPECT_TRUE(ImmutableString("x"sv).holds_copy());
  EXPECT_TRUE(ImmutableString(3, 'x').holds_copy());
  EXPECT_FALSE(ImmutableString().holds_copy());
  EXPECT_FALSE(ImmutableString::ref("x").holds_copy());

}

TEST(ImmutableStringTest, U32Construction) {
  using IS = ImmutableBasicString<char32_t>;

  EXPECT_TRUE(IS(U"x").holds_copy());
  EXPECT_TRUE(IS(U"x"sv).holds_copy());
  EXPECT_TRUE(IS(3, U'x').holds_copy());
  EXPECT_FALSE(IS().holds_copy());
  EXPECT_FALSE(IS::ref(U"x").holds_copy());

}

TEST(ImmutableStringTest, Accessors) {

  ImmutableString is;

  EXPECT_EQ(0, is.size());
  EXPECT_EQ(nullptr, is.data());

  is = ImmutableString("good");
  EXPECT_EQ(4, is.size());
  EXPECT_EQ("good"s, is.copy());
  EXPECT_EQ("good"sv, is.view());

  is = ImmutableString(ImmutableString::REF, "hello");
  EXPECT_EQ(5, is.size());
  EXPECT_EQ("hello"s, is.copy());
  EXPECT_EQ("hello"sv, is.view());

}

} // namespace cbu
