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

#include "cbu/common/fifo_list.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace cbu {
inline namespace cbu_fifo_list {
namespace {

TEST(FifoListTest, FifoListTest) {
  fifo_list<std::string> a;

  ASSERT_TRUE(a.empty());

  a.emplace_back(5, 'a');
  // {"aaaaa"}
  ASSERT_FALSE(a.empty());
  ASSERT_EQ(a.front(), "aaaaa");
  ASSERT_EQ(a.back(), "aaaaa");

  a.emplace_back("hello");
  // {"aaaaa", "hello"}
  ASSERT_FALSE(a.empty());
  ASSERT_EQ(a.front(), "aaaaa");
  ASSERT_EQ(a.back(), "hello");

  std::vector<std::string> v(a.begin(), a.end());
  ASSERT_EQ(v, std::vector<std::string>({"aaaaa", "hello"}));

  a.pop_front();
  // {"hello"}
  ASSERT_FALSE(a.empty());
  ASSERT_EQ(a.front(), "hello");
  ASSERT_EQ(a.back(), "hello");

  a.pop_front();
  // {}
  ASSERT_TRUE(a.empty());
}

} // namespace
} // inline namespace cbu_fifo_list
} // namespace cbu
