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

#include "cbu/common/strutil.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "cbu/debug/gtest_formatters.h"

namespace cbu {

using namespace std::literals;

TEST(StrUtilTest, Scnprintf) {
  char buf[16];

  EXPECT_EQ(0, scnprintf(buf, 0, "%s", "abcdefghijklmnopqrstuvwxyz"));
  EXPECT_EQ(0, scnprintf(buf, 1, "%s", "abcdefghijklmnopqrstuvwxyz"));
  EXPECT_EQ(1, scnprintf(buf, 2, "%s", "abcdefghijklmnopqrstuvwxyz"));
}

TEST(StrUtilTest, Strcnt) {
  EXPECT_EQ(0, strcnt("abcabc", '\0'));
  EXPECT_EQ(2, strcnt("abcabc", 'a'));
  EXPECT_EQ(0, strcnt("abcabc", 'x'));
  EXPECT_EQ(4, strcnt("0123456789abcdef012345678901234567890123456789", '9'));
}

TEST(StrUtilTest, Memcnt) {
  EXPECT_EQ(0, memcnt("abcabc"sv, '\0'));
  EXPECT_EQ(2, memcnt("abcabc"sv, 'a'));
  EXPECT_EQ(0, memcnt("abcabc"sv, 'x'));
  EXPECT_EQ(4, memcnt("0123456789abcdef012345678901234567890123456789"sv, '9'));
  EXPECT_EQ(
      9,
      memcnt(
          "0123456789abcdef01234567890123456789012345678901234567890123456789012345678999"sv,
          '9'));
}

TEST(StrUtilTest, Reverse) {
  char buf[] = "abcdefghijklmnopqrstuvwxyz";
  EXPECT_EQ(std::end(buf) - 1, reverse(std::begin(buf), std::end(buf) - 1));
  EXPECT_EQ("zyxwvutsrqponmlkjihgfedcba", std::string(buf));
}

TEST(StrUtilTest, StrNumCmp) {
  EXPECT_LT(0, strnumcmp("abcd12a", "abcd9a"));
  EXPECT_EQ(0, strnumcmp("abcd12a", "abcd12a"));
  EXPECT_GT(0, strnumcmp("abcd12a", "abcd23a"));
}

TEST(StrUtilTest, AnyToStringView) {
  EXPECT_EQ(any_to_string_view("xyz"), "xyz"sv);
  EXPECT_EQ(any_to_string_view((const char8_t*)u8"一二"), u8"一二"sv);
  EXPECT_EQ(any_to_string_view(u8"一二"), u8"一二"sv);
  EXPECT_EQ(any_to_string_view(std::u16string_view(u"一二")), u"一二"sv);
  std::u32string s = U"🀄️";
  EXPECT_EQ(any_to_string_view(s), U"🀄️"sv);
}

TEST(StrUtilTest, StrCmpLengthFirst) {
  EXPECT_LT(strcmp_length_first("z", "ab"), 0);
  EXPECT_LT(strcmp_length_first("ab", "zz"), 0);
  EXPECT_EQ(strcmp_length_first("ab", "ab"), 0);
  EXPECT_GT(strcmp_length_first("zz", "ab"), 0);
  EXPECT_GT(strcmp_length_first("zzz", "ab"), 0);
  EXPECT_GT(strcmp_length_first("aaa", "zz"), 0);

  EXPECT_GT(strcmp_length_first("中"sv, "xy"sv), 0);
  EXPECT_GT(strcmp_length_first(u8"中"sv, u8"xy"sv), 0);
  EXPECT_LT(strcmp_length_first(L"中"sv, L"xy"s), 0);
  EXPECT_LT(strcmp_length_first(u"中", u"xy"sv), 0);
  EXPECT_LT(strcmp_length_first(U"中"sv, U"xy"sv), 0);
}

inline size_t naive_common_prefix(std::string_view a, std::string_view b) {
  size_t k = 0;
  while (k < a.length() && k < b.length() && a[k] == b[k]) ++k;
  return k;
}

TEST(StrUtilTest, CommonPrefix) {
  constexpr unsigned N = 4096;
  char bytes[N];
  srand(0);

  for (size_t i = 0; i < N; ++i) bytes[i] = 'a' + (i & 3);
  for (size_t i = 0; i < N; ++i) {
    size_t k = rand() % (N - i);
    memmove(bytes + i + k, bytes + i, N - i - k);

    std::string_view a = {bytes + i, N - i};
    for (size_t j = i + 1; j < N; ++j) {
      std::string_view b = {bytes + j, N - j};
      size_t naive = naive_common_prefix(a, b);
      ASSERT_EQ(naive, common_prefix(a, b));
    }
  }
}

TEST(StrUtilTest, CommonPrefix_Utf8) {
  EXPECT_EQ(common_prefix(u8"一丁", u8"一一"), 3);
  EXPECT_EQ(common_prefix((const char*)u8"一丁", (const char*)u8"一一"), 5);
}

inline size_t naive_common_suffix(std::string_view a, std::string_view b) {
  size_t k = 0;
  while (k < a.length() && k < b.length() && a.end()[-k - 1] == b.end()[-k - 1])
    ++k;
  return k;
}

TEST(StrUtilTest, CommonSuffix) {
  constexpr unsigned N = 4096;
  char bytes[N];
  srand(0);

  for (size_t i = 0; i < N; ++i) bytes[i] = 'a' + (i & 3);
  for (size_t i = 0; i < N; ++i) {
    size_t k = rand() % (N - i);
    memmove(bytes + i, bytes + i + k, N - i - k);

    std::string_view a = {bytes, i};
    for (size_t j = i + 1; j < N; ++j) {
      std::string_view b = {bytes, j};
      size_t naive = naive_common_suffix(a, b);
      ASSERT_EQ(naive, common_suffix(a, b));
    }
  }
}

TEST(StrUtilTest, CommonSuffix_Utf8) {
  EXPECT_EQ(common_suffix(u8"帀一", u8"一一"), 3);
  EXPECT_EQ(common_suffix((const char*)u8"帀一", (const char*)u8"一一"), 5);
}

TEST(StrUtilTest, CharSpanLength) {
  constexpr unsigned N = 4096;
  char bytes[N];
  std::fill_n(bytes, std::size(bytes), 'b');

  for (size_t m = 0; m < 128; ++m) {
    for (size_t i = 0; i < 1024; ++i) {
      for (size_t j = i; j < N; j += j / 2 + 1) {
        size_t expected = m <= i ? 0 : std::min(j, m) - i;
        ASSERT_EQ(char_span_length(bytes + i, j - i, 'a'), expected)
            << "m = " << m << " i = " << i << " j = " << j;
      }
    }
    bytes[m] = 'a';
  }
}

}  // namespace cbu
