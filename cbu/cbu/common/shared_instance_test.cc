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

#include "cbu/common/shared_instance.h"

#include <string>
#include <utility>

#include "gtest/gtest.h"

namespace cbu {

TEST(SharedInstanceTest, Equality) {
  EXPECT_EQ(0, (shared<int, 0>()));
  EXPECT_EQ(5, (shared<int, 5>()));
  EXPECT_EQ("xxxxx", (shared<const std::string, 5, 'x'>()));
}

TEST(SharedInstanceTest, Uniqueness) {
  EXPECT_EQ((&shared<std::string, 5, 'x'>()),
            (&shared<std::string, 5, 'x'>()));
  EXPECT_NE((&shared<std::string, 5, 'x'>()),
            (&shared<const std::string, 5, 'x'>()));
}

#if defined __cpp_constinit && __cpp_constinit >= 201907
TEST(SharedInstanceTest, ConstInit) {
  EXPECT_EQ(0, (shared_constinit<int, 0>()));
  EXPECT_EQ(5, (shared_constinit<int, 5>()));
}
#endif

TEST(SharedInstanceTest, ConstexprTest) {
  constexpr const std::pair<int, int>& p =
      shared_const<std::pair<int, int>, 25, 54>;
  static_assert(p.first == 25);
  static_assert(p.second == 54);
}

} // namespace cbu
