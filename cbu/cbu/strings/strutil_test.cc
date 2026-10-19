/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
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

#include "cbu/strings/strutil.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "cbu/debug/gtest_formatters.h"

namespace cbu {

using namespace std::literals;

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
#define REPEAT8(x) x x x x x x x x
  EXPECT_EQ(72, memcnt(std::string_view(REPEAT8(
                           "0123456789abcdef0123456789012345678901234567890"
                           "1234567890123456789012345678999")),
                       '9'));
#undef REPEAT8
  char buf[1024];
  for (int i = 0; i < 1024; ++i) {
    buf[i] = i % 8;
  }
  for (size_t offset = 0; offset < 32; ++offset) {
    for (size_t end = 1024 - 32; end < 1024; ++end) {
      size_t zeros = (end + 7) / 8 - (offset + 7) / 8;
      ASSERT_EQ(memcnt(buf + offset, '\0', end - offset), zeros);
    }
  }
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

  EXPECT_LT(0, strnumcmp("abcd12a"sv, "abcd9a"sv));
  EXPECT_EQ(0, strnumcmp("abcd12a"sv, "abcd12a"sv));
  EXPECT_GT(0, strnumcmp("abcd12a"sv, "abcd23a"sv));
  EXPECT_GT(strnumcmp("abcd12a"sv, "abcd3"sv), 0);
  EXPECT_GT(strnumcmp("abcd12a"sv, "abcd"sv), 0);
  EXPECT_GT(strnumcmp("abcd12a"sv, "abcdx"sv), 0);
  EXPECT_EQ(strnumcmp("abcd123"sv, "abcd123"sv), 0);
  EXPECT_LT(strnumcmp("abcd123"sv, "abcd123x"sv), 0);
  EXPECT_GT(strnumcmp("abcd123"sv, "abcd103x"sv), 0);
  EXPECT_GT(strnumcmp("abcd123"sv, "abcd103"sv), 0);
  EXPECT_LT(strnumcmp("abcd123"sv, "abcd0230"sv), 0);
  EXPECT_GT(strnumcmp("abcd0230"sv, "abcd123"sv), 0);
  EXPECT_GT(strnumcmp("abcd123x"sv, "abcd123"sv), 0);
  EXPECT_LT(strnumcmp("abcd103x"sv, "abcd123"sv), 0);
  EXPECT_GT(strnumcmp("2abcdefghijklmnopqrstuvwxyz1"sv, "1abcdefghijklmnopqrstuvwxyz2"sv), 0);

  // SIMD test
#define LS "A long long string is here"
  EXPECT_GT(strnumcmp(LS "abcd12a"sv, LS "abcd3"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd12a"sv, LS "abcd"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd12a"sv, LS "abcdx"sv), 0);
  EXPECT_EQ(strnumcmp(LS "abcd123"sv, LS "abcd123"sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd123"sv, LS "abcd123x"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123"sv, LS "abcd103x"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123"sv, LS "abcd103"sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd123"sv, LS "abcd0230"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd0230"sv, LS "abcd123"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123x"sv, LS "abcd123"sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd103x"sv, LS "abcd123"sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd12a" LS ""sv, LS "abcd3" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd12a" LS ""sv, LS "abcd" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd12a" LS ""sv, LS "abcdx" LS ""sv), 0);
  EXPECT_EQ(strnumcmp(LS "abcd123" LS ""sv, LS "abcd123" LS ""sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd123" LS ""sv, LS "abcd123x" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123" LS ""sv, LS "abcd103x" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123" LS ""sv, LS "abcd103" LS ""sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd123" LS ""sv, LS "abcd0230" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd0230" LS ""sv, LS "abcd123" LS ""sv), 0);
  EXPECT_GT(strnumcmp(LS "abcd123x" LS ""sv, LS "abcd123" LS ""sv), 0);
  EXPECT_LT(strnumcmp(LS "abcd103x" LS ""sv, LS "abcd123" LS ""sv), 0);
#undef LS
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
  EXPECT_EQ(common_prefix(u8"一不", u8"一一"), 3);
  EXPECT_EQ(common_prefix((const char*)u8"一不", (const char*)u8"一一"), 5);
}

TEST(StrUtilTest, CommonPrefix_EmptyInput) {
  // Default-constructed string_views have null data()
  std::string_view empty;
  EXPECT_EQ(common_prefix(empty, empty), 0);
  EXPECT_EQ(common_prefix(empty, "abc"sv), 0);
  EXPECT_EQ(common_prefix("abc"sv, empty), 0);
  EXPECT_EQ(common_prefix_utf8(empty, empty), 0);
}

inline size_t naive_common_suffix(std::string_view a, std::string_view b) {
  size_t k = 0;
  // Use indexing instead of reverse iterators/end()-relative access:
  // clang 22 on aarch64 miscompiles the latter when libc++ headers are in
  // play, turning a computed address into a hardcoded -1.
  while (k < a.length() && k < b.length() &&
         a[a.length() - 1 - k] == b[b.length() - 1 - k])
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

TEST(StrUtilTest, TruncateStringUtf8) {
  EXPECT_EQ(truncate_string_utf8("abcd"sv, 3), "abc"sv);
  EXPECT_EQ(truncate_string_utf8("abc"sv, 3), "abc"sv);
  EXPECT_EQ(truncate_string_utf8("ab"sv, 3), "ab"sv);
  EXPECT_EQ(truncate_string_utf8("一二"sv, 3), "一"sv);
  EXPECT_EQ(truncate_string_utf8("一二"sv, 5), "一"sv);
  EXPECT_EQ(truncate_string_utf8("一二"sv, 6), "一二"sv);
  EXPECT_EQ(truncate_string_utf8("一二"sv, 8), "一二"sv);
  EXPECT_EQ(truncate_string_utf8("一二"sv, 2), ""sv);
}

TEST(StrUtilTest, CharSpanLength) {
  constexpr unsigned N = 4096;
  char bytes[N];
  std::fill_n(bytes, std::size(bytes), 'b');

  for (size_t m = 0; m < 128; ++m) {
    // bytes[0..m-1] = 'a'
    // bytes[m..] = 'b'
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

TEST(StrUtilTest, CharSpanLength_EmptyInput) {
  // Default-constructed string_view has null data()
  std::string_view empty;
  EXPECT_EQ(char_span_length(empty, 'a'), 0);
  EXPECT_EQ(char_span_length(empty, '\0'), 0);
}

}  // namespace cbu
