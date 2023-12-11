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
  memdrop(buf_, bswap_for<std::endian::little>(0x0123456789abcdefull), 3);
  EXPECT_EQ(static_cast<char>(0xef), buf_[0]);
  EXPECT_EQ(static_cast<char>(0xcd), buf_[1]);
  EXPECT_EQ(static_cast<char>(0xab), buf_[2]);
  EXPECT_EQ(static_cast<char>(0x03), buf_[3]);

#if __WORDSIZE >= 64
  if (std::endian::native == std::endian::little) {
    unsigned __int128 v =
      ((unsigned __int128)UINT64_C(0x0102030405060708) << 64) |
      UINT64_C(0x090a0b0c0d0e0f);

    memset(buf_, 0, sizeof(buf_));
    memdrop(buf_, v, 7);
    EXPECT_EQ(std::string(buf_, 7), std::string((char*)&v, 7));
    EXPECT_EQ(buf_[7], 0);

    memset(buf_, 0, sizeof(buf_));
    memdrop(buf_, v, 14);
    EXPECT_EQ(std::string(buf_, 14), std::string((char*)&v, 14));
    EXPECT_EQ(buf_[14], 0);
  }
#endif

#ifdef __SSE2__
  {
    __m128i x = _mm_setr_epi8(
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    for (int i = 0; i <= 16; ++i) {
      memset(buf_, 42, 16);
      ASSERT_EQ(buf_ + i, memdrop(buf_, x, i));
      for (int j = 0; j < 16; ++j) {
        ASSERT_EQ((j < i) ? j : 42, buf_[j]);
      }
    }
  }
#endif

#ifdef __AVX2__
  {
    __m256i x = _mm256_setr_epi8(
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
    for (int i = 0; i <= 32; ++i) {
      memset(buf_, 42, 32);
      ASSERT_EQ(buf_ + i, memdrop(buf_, x, i)) << "length=" << i;
      for (int j = 0; j < 32; ++j) {
        ASSERT_EQ((j < i) ? j : 42, buf_[j]) << "length=" << i << ", j=" << j;
      }
    }
  }
#endif
}

TEST(MemPickDropConstexprTest, MemDrop) {
  struct A {
    constexpr A() {
      char* s = buf_;
      s = memdrop_be<uint32_t>(s, 0x01020304);
      s = memdrop_le<uint32_t>(s, 0x01020304);
      l_ = s - buf_;
    }

    char buf_[8];
    size_t l_;
  };
  constexpr A a;
  static_assert(a.l_ == 8);
  static_assert(a.buf_[0] == 1);
  static_assert(a.buf_[1] == 2);
  static_assert(a.buf_[2] == 3);
  static_assert(a.buf_[3] == 4);
  static_assert(a.buf_[4] == 4);
  static_assert(a.buf_[5] == 3);
  static_assert(a.buf_[6] == 2);
  static_assert(a.buf_[7] == 1);
}

TEST(MemPickDropConstexprTest, MemPick) {
  constexpr char b[] {1, 2, 3, 4, 4, 3, 2, 1};
  static_assert(mempick_be<uint32_t>(b) == 0x01020304);
  static_assert(mempick_le<uint32_t>(b + 4) == 0x01020304);
}

TEST(MemPickDropConstexprTest, MemDropNumber) {
  struct A {
    constexpr A() {
      for (size_t i = 0; i < std::size(buf_); ++i) {
        buf_[i] = static_cast<uint8_t>(i);
      }
      memdrop(buf_, bswap_for<std::endian::little>(0x0123456789abcdefull), 3);
    }
    uint8_t buf_[8];
  };
  constexpr A a;
  static_assert(a.buf_[0] == 0xef);
  static_assert(a.buf_[1] == 0xcd);
  static_assert(a.buf_[2] == 0xab);
  static_assert(a.buf_[3] == 0x03);
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
  append(&u, {L" ", L"world"});
  EXPECT_EQ(L"Hello world"sv, u);
  append(&u, {L", "});
  EXPECT_EQ(L"Hello world, "sv, u);
  append(&u, {});
  EXPECT_EQ(L"Hello world, "sv, u);
  append(&u, {L"a", L"b", L"c"});
  EXPECT_EQ(L"Hello world, abc"sv, u);
}

TEST(FastStrTest, Concat) {
  EXPECT_EQ("Hello world"sv,
            concat({"Hello", " ", "world"}));
  EXPECT_EQ(L"Hello world"sv,
            concat({L"Hello", L" ", L"world"}));
}

} // namespace cbu
