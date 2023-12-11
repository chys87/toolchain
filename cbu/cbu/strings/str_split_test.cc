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

#include "cbu/strings/str_split.h"

#include <queue>
#include <set>
#include <vector>

#include <gtest/gtest.h>

namespace cbu {
namespace {

using std::operator""sv;

using V = std::vector<std::string_view>;

struct AppendToVec {
  V* vec;
  void operator()(std::string_view part) const { vec->push_back(part); }
};

struct Count {
  int* cnt;
  void operator()(std::string_view) const noexcept { ++*cnt; }
};

TEST(StrSplitTest, SplitByChar) {
  {
    V vec;
    ASSERT_TRUE(StrSplit("We the People", ' ', AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"We", "the", "People"}));
  }
  {
    V vec;
    ASSERT_TRUE(StrSplit("We the People", ByChar(' '), AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"We", "the", "People"}));
  }
  {
    V vec;
    ASSERT_TRUE(StrSplit("A  B C", ' ', AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"A", "", "B", "C"}));
  }
}

TEST(StrSplitTest, SplitByStringView) {
  {
    V vec;
    ASSERT_TRUE(StrSplit("a = b <= c < d", "<=", AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"a = b ", " c < d"}));
  }
  {
    V vec;
    ASSERT_TRUE(
        StrSplit("a = b <= c < d", ByStringView("<="), AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"a = b ", " c < d"}));
  }
}

TEST(StrSplitTest, ReturnsFalse) {
  int cnt = 0;
  ASSERT_FALSE(StrSplit("1.2.3.4", '.', [&](std::string_view) {
    ++cnt;
    return cnt < 2;
  }));
  EXPECT_EQ(cnt, 2);
}

TEST(StrSplitTest, Empty) {
  {
    int cnt = 0;
    ASSERT_TRUE(StrSplit("", '.', Count{&cnt}));
    EXPECT_EQ(cnt, 0);
  }

  {
    int cnt = 0;
    ASSERT_TRUE(StrSplit("", ".", Count{&cnt}));
    EXPECT_EQ(cnt, 0);
  }

  {
    int cnt = 0;
    ASSERT_TRUE(StrSplit("", ".", Count{&cnt}));
    EXPECT_EQ(cnt, 0);
  }

  {
    V vec;
    ASSERT_TRUE(StrSplit("abc", "", AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"a", "b", "c"}));
  }
}

TEST(StrSplitTest, Predicate) {
  {
    V vec;
    ASSERT_TRUE(StrSplit(",a,b,", ',', AllowEmpty(), AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"", "a", "b", ""}));
  }
  {
    V vec;
    ASSERT_TRUE(StrSplit(",a,b,", ',', SkipEmpty(), AppendToVec{&vec}));
    EXPECT_EQ(vec, V({"a", "b"}));
  }
}

TEST(StrSplitTest, Container) {
  {
    V vec;
    StrSplitTo(",a,b,", ',', &vec);
    EXPECT_EQ(vec, V({"", "a", "b", ""}));
  }
  {
    V vec;
    StrSplitTo(",a,b,", ',', SkipEmpty(), &vec);
    EXPECT_EQ(vec, V({"a", "b"}));
  }
  ASSERT_EQ(StrSplitTo<V>(",a,b,", ','), V({"", "a", "b", ""}));
  ASSERT_EQ(StrSplitTo<V>(",a,b,", ',', SkipEmpty()), V({"a", "b"}));

  ASSERT_EQ(StrSplitTo<std::set<std::string_view>>(",a,b,", ',', SkipEmpty()),
            std::set({"a"sv, "b"sv}));

  ASSERT_EQ(StrSplitTo<std::queue<std::string_view>>(",a,b,", ',', SkipEmpty()),
            std::queue(std::deque{"a"sv, "b"sv}));
}

}  // namespace
}  // namespace cbu
