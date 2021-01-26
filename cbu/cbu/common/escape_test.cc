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

#include "escape.h"
#include <gtest/gtest.h>

namespace cbu {

TEST(EscapeTest, EscapeC) {
  std::string s;
  escape_string_append(&s, "\1\2\b\f\\\"Hello world一二三四五六七八九\\/");
  EXPECT_EQ(s, R"(\x01\x02\b\f\\\"Hello world一二三四五六七八九\\/)");
}

TEST(EscapeTest, EscapeJSON) {
  std::string s;
  escape_string_append(&s, "\1\2\b\f\\\"Hello world一二三四五六七八九\\/",
                       EscapeStyle::JSON);
  EXPECT_EQ(s, R"(\u0001\u0002\b\f\\\"Hello world一二三四五六七八九\\\/)");
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

} // namespace cbu
