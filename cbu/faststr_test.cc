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

#include "faststr.h"
#include <gtest/gtest.h>

namespace cbu {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

class MemPickDropTest : public testing::Test {
 public:
  void SetUp() override {
    for (size_t i = 0; i < std::size(buf_); ++i) {
      buf_[i] = static_cast<char>(i);
    }
  }

 protected:
  char buf_[128];
};

TEST_F(MemPickDropTest, MemPick) {
  EXPECT_EQ(0x0100, mempick_le<uint16_t>(buf_));
  EXPECT_EQ(0x03020100, mempick_le<uint32_t>(buf_));
  EXPECT_EQ(0x0001, mempick_be<uint16_t>(buf_));
  EXPECT_EQ(0x00010203, mempick_be<uint32_t>(buf_));
}

TEST_F(MemPickDropTest, MemDrop) {
  memdrop_le<uint32_t>(buf_, 0xaabbccdd);
  EXPECT_EQ(static_cast<char>(0xdd), buf_[0]);
  EXPECT_EQ(static_cast<char>(0xcc), buf_[1]);
}

TEST_F(MemPickDropTest, MemDropNumber) {
  memdrop(buf_, 0x0123456789abcdefull, 3);
  EXPECT_EQ(static_cast<char>(0xef), buf_[0]);
  EXPECT_EQ(static_cast<char>(0xcd), buf_[1]);
  EXPECT_EQ(static_cast<char>(0xab), buf_[2]);
  EXPECT_EQ(static_cast<char>(0x03), buf_[3]);
}

TEST(FastStrTest, NPrintf) {
  std::string u;
  EXPECT_EQ(11, append_nprintf(&u, 32, "Hello %s", "world"));
  EXPECT_EQ("Hello world"sv, u);

  u.clear();
  EXPECT_EQ(11, append_nprintf(&u, 2, "Hello %s", "world"));
  EXPECT_EQ("Hello world"sv, u);

  EXPECT_EQ("Hello world"sv, nprintf(2, "Hello %s", "world"));
}

TEST(FastStrTest, Append) {
  std::wstring u = L"Hello";
  append(&u, {L" "sv, L"world"sv});
  EXPECT_EQ(L"Hello world"sv, u);
}

TEST(FastStrTest, Concat) {
  EXPECT_EQ(U"Hello world"sv,
            concat({U"Hello"sv, U" "sv, U"world"sv}));
}

} // namespace cbu
