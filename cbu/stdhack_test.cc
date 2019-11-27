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

#include "stdhack.h"
#include <gtest/gtest.h>
#include <string_view>

// Hacks standard containers to add more functionality

namespace cbu {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

TEST(StdHackTest, Extend) {
  std::string u;
  char *w = extend(&u, 1);

  ASSERT_EQ(1, u.size());
  EXPECT_EQ(u.data(), w);
  EXPECT_EQ('\0', u[1]);

  w = extend(&u, 5);
  ASSERT_EQ(6, u.size());
  EXPECT_EQ(u.data() + 1, w);
  EXPECT_EQ('\0', u[6]);
}

TEST(StdHackTest, Truncate) {
  std::string u = "abcdefg"s;
  truncate(&u, 16);
  EXPECT_EQ(7, u.length());
  truncate(&u, 4);
  EXPECT_EQ("abcd"sv, u);
}

TEST(StdHackTest, TruncateUnsafe) {
  std::string u = "abcdefghijklmn"s;
  truncate_unsafe(&u, 4);
  EXPECT_EQ("abcd"sv, u);
}

TEST(StdHackTest, TruncateUnsafer) {
  std::string u = "abcd\0efgh"s;
  EXPECT_EQ(9, u.length());
  truncate_unsafer(&u, 4);
  EXPECT_EQ("abcd"sv, u);
}

} // namespace cbu
