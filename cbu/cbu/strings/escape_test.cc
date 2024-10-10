/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include "escape.h"
#include <gtest/gtest.h>

namespace cbu {
namespace {

TEST(EscapeTest, EscapeC) {
  std::string s;
  escape_string_append(&s, "\1\2\a\b\v\f\\\"Hello world一二三四五六七八九\\/");
  EXPECT_EQ(s, R"(\x01\x02\a\b\v\f\\\"Hello world一二三四五六七八九\\/)");

  EXPECT_EQ(escape_string(""), "");
}

TEST(EscapeTest, EscapeJSON) {
  std::string s;
  escape_string_append<EscapeStyle::JSON>(
      &s, "\1\2\a\b\v\f\\\"Hello world一二三四五六七八九\\/");
  EXPECT_EQ(
      s, R"(\u0001\u0002\u0007\b\u000b\f\\\"Hello world一二三四五六七八九\\/)");
  s.clear();
  escape_string_append<EscapeStyle::JSON_STRICT>(
      &s, "\1\2\a\b\v\f\\\"Hello world一二三四五六七八九\\/");
  EXPECT_EQ(
      s,
      R"(\u0001\u0002\u0007\b\u000b\f\\\"Hello world一二三四五六七八九\\\/)");

  EXPECT_EQ(escape_string<EscapeStyle::JSON>("\1\2😍"), R"(\u0001\u0002😍)");
}

TEST(EscapeTest, AlignmentTest) {
  constexpr int N = 1024;
  alignas(32) char buf[N];
  for (int i = 0; i < N; ++i) {
    if (i % 71 < 35) {
      buf[i] = i;
    } else {
      buf[i] = 'A' + (i % 26);
    }
  }

  alignas(32) char buf2[N];

  for (int start = 0; start < 32; ++start) {
    for (int end = start; end < N; ) {
      std::string_view sv(buf + start, end - start);
      memcpy(buf2, sv.data(), sv.size());
      ASSERT_EQ(escape_string(sv), escape_string({buf2, sv.size()})) <<
        "start=" << start << " end=" << end;

      if (end - start < 64 || N - end < 64) {
        ++end;
      } else {
        end += end / 3;
      }
    }
  }
}

std::string unescape(std::string_view s) {
  char buffer[1024];
  auto [status, dst, src] = unescape_string(buffer, s);
  if (status == UnescapeStringStatus::OK_EOS ||
      status == UnescapeStringStatus::OK_QUOTE) {
    return std::string(buffer, dst - buffer);
  } else {
    return {};
  }
}

TEST(EscapeTest, UnEscape) {
  char buffer[512];

  // Basic tests
  EXPECT_EQ(unescape_string(buffer, R"(\)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\x)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\x1)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\x12)").status,
            UnescapeStringStatus::OK_EOS);
  EXPECT_EQ(unescape_string(buffer, R"(\x1x)").status,
            UnescapeStringStatus::INVALID_ESCAPE);

  EXPECT_EQ(unescape_string(buffer, R"(\u)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\uabc)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\uabcd)").status,
            UnescapeStringStatus::OK_EOS);
  EXPECT_EQ(unescape_string(buffer, R"(\uabcu)").status,
            UnescapeStringStatus::INVALID_ESCAPE);

  EXPECT_EQ(unescape_string(buffer, R"(\U)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\U0001abc)").status,
            UnescapeStringStatus::INVALID_ESCAPE);
  EXPECT_EQ(unescape_string(buffer, R"(\U0001abcd)").status,
            UnescapeStringStatus::OK_EOS);
  EXPECT_EQ(unescape_string(buffer, R"(\U0001abcU)").status,
            UnescapeStringStatus::INVALID_ESCAPE);

  // Surrogate pairs
  EXPECT_EQ(unescape_string(buffer, R"(\udc00)").status,
            UnescapeStringStatus::TAIL_SURROGATE_WITHOUT_HEAD);
  EXPECT_EQ(unescape_string(buffer, R"(\ud800)").status,
            UnescapeStringStatus::HEAD_SURROGATE_WITHOUT_TAIL);
  EXPECT_EQ(unescape_string(buffer, R"(\ud800\udbff)").status,
            UnescapeStringStatus::HEAD_SURROGATE_WITHOUT_TAIL);
  EXPECT_EQ(unescape_string(buffer, R"(\ud800\udc00)").status,
            UnescapeStringStatus::OK_EOS);
  EXPECT_EQ(std::string(buffer, 4), std::string((const char*)u8"\U00010000"));

  // Out of range
  EXPECT_EQ(unescape_string(buffer, R"(\U0000dc00)").status,
            UnescapeStringStatus::CODE_POINT_OUT_OF_RANGE);
  EXPECT_EQ(unescape_string(buffer, R"(\U0000dfff)").status,
            UnescapeStringStatus::CODE_POINT_OUT_OF_RANGE);
  EXPECT_EQ(unescape_string(buffer, R"(\U00110000)").status,
            UnescapeStringStatus::CODE_POINT_OUT_OF_RANGE);

  // Legal sequences
  ASSERT_EQ(unescape(R"(\1\11\111\377\400\x3a)"), "\1\11\111\377\0400\x3a");
  ASSERT_EQ(unescape(R"(\uabcd\U0001abcd)"), "\uabcd\U0001abcd");
  ASSERT_EQ(unescape(R"(Hello world\"\"", )"), "Hello world\"\"");

  ASSERT_EQ(unescape(R"(abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ)"),
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
  ASSERT_EQ(unescape(R"(abcdefghijklmnopqrstuvwxyz\vABCDEFGHIJKLMNOPQRSTUVW)"),
            "abcdefghijklmnopqrstuvwxyz\vABCDEFGHIJKLMNOPQRSTUVW");
}

TEST(EscapeTest, UnEscapeInPlace) {
  char buffer[] = R"(\uabcd\u1234\u1111\u2222\u3333\u4444\u5555\u6666)";
  const char* end = buffer + strlen(buffer);
  auto [status, end_dst, end_src] = unescape_string(buffer, buffer, end);
  ASSERT_EQ(status, UnescapeStringStatus::OK_EOS);
  ASSERT_EQ(end_src, end);
  ASSERT_EQ(std::string(buffer, end_dst),
            "\uabcd\u1234\u1111\u2222\u3333\u4444\u5555\u6666");
}

} // namespace
} // namespace cbu
