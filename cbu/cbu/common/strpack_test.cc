/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2020, chys <admin@CHYS.INFO>
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

#include "strpack.h"
#include <gtest/gtest.h>

namespace cbu {
namespace {

TEST(StrPackTest, MembersTest) {
  auto t = "abc"_strpack;
  EXPECT_EQ(3, t.n);
  EXPECT_STREQ("abc", t.s);

  auto w = L"def"_strpack;
  EXPECT_EQ(3, w.n);
  EXPECT_EQ(4 * sizeof(wchar_t), sizeof(w.s));
  EXPECT_EQ(L'd', w.s[0]);
  EXPECT_EQ(L'e', w.s[1]);
  EXPECT_EQ(L'f', w.s[2]);
  EXPECT_EQ(L'\0', w.s[3]);
}

TEST(StrPackTest, FillTest) {
  char buffer[5];
  *("abcd"_strpack.fill(buffer)) = '\0';
  EXPECT_STREQ("abcd", buffer);
}

template <const char* s>
void test_foo(char* d) {
  strcpy(d, s);
}

TEST(StrPackTest, TemplateArgumentTest) {
  char d[128];
  test_foo<"Good morning"_str>(d);
  EXPECT_STREQ("Good morning", d);
}

} // namespace
} // namespace cbu
