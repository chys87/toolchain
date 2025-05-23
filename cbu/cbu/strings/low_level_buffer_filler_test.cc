/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2025, chys <admin@CHYS.INFO>
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

#include "cbu/strings/low_level_buffer_filler.h"

#include <gtest/gtest.h>

namespace cbu {
namespace {

using std::operator""sv;

TEST(LowLevelBufferFillerTest, NonConstExpr) {
  // Hopefully this makes LowLevelBufferFiller not constexpr
  std::unique_ptr<char[]> buffer(new char[4096]);

  LowLevelBufferFiller filler{buffer.get()};
  filler << 'x'
    << "yz"sv
    << FillLittleEndian<uint16_t>(uint16_t(258))
    << FillBigEndian<uint16_t>(uint16_t(258));

  char* p = filler.pointer();
  auto size = p - buffer.get();
  EXPECT_EQ(7, size);
  EXPECT_EQ("xyz\x02\x01\x01\x02"sv, std::string_view(buffer.get(), size));
}

struct StaticBuffer {
  char buffer[128];
  size_t n;
};

TEST(LowLevelBufferFillerTest, ConstExpr) {
  constexpr StaticBuffer sb = []() constexpr noexcept {
    StaticBuffer res{};
    LowLevelBufferFiller filler{res.buffer};
    filler << 'x'
      << "yz"sv
      << FillLittleEndian<uint16_t>(uint16_t(258))
      << FillBigEndian<uint16_t>(uint16_t(258));
    res.n = filler.pointer() - res.buffer;
    return res;
  }();
  static_assert(7 == sb.n);
  EXPECT_EQ("xyz\x02\x01\x01\x02"sv, std::string_view(sb.buffer, sb.n));
}

TEST(LowLevelBufferFillerTest, FillDec) {

  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<0>(12345);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 5);
    ASSERT_EQ(std::string(buffer, p - buffer), "12345");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<9>(5);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 1);
    ASSERT_EQ(std::string(buffer, p - buffer), "5");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<20>(15);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "15");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<99>(54);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "54");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<999>(254);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 3);
    ASSERT_EQ(std::string(buffer, p - buffer), "254");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<99999>(12345);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 5);
    ASSERT_EQ(std::string(buffer, p - buffer), "12345");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<88, FillOptions{.width = 2, .fill = '!'}>(7);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "!7");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<88, FillOptions{.width = 2, .fill = '!'}>(57);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "57");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<99999, FillOptions{.width = 5}>(123);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 5);
    ASSERT_EQ(std::string(buffer, p - buffer), "00123");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<99999, FillOptions{.width = 5, .fill = '@'}>(123);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 5);
    ASSERT_EQ(std::string(buffer, p - buffer), "@@123");
  }
  // Not using kDigits99
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<130, FillOptions{.width = 2}>(132);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "32");
  }
  // Using kDigits99
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<100, FillOptions{.width = 2}>(17);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "17");
  }
  // optimize_size
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<10, FillOptions{.optimize_size = true}>(5);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 1);
    ASSERT_EQ(std::string(buffer, p - buffer), "5");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<20, FillOptions{.optimize_size = true}>(15);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "15");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<100, FillOptions{.optimize_size = true}>(85);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 2);
    ASSERT_EQ(std::string(buffer, p - buffer), "85");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<1000, FillOptions{.optimize_size = true}>(254);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 3);
    ASSERT_EQ(std::string(buffer, p - buffer), "254");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<0x1'0000'0000, FillOptions{.optimize_size = true}>(
        4294967295);
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 10);
    ASSERT_EQ(std::string(buffer, p - buffer), "4294967295");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillDec<0, FillOptions{.optimize_size = true}>(
        UINT64_C(18446744073709551615));
    char* p = filler.pointer();
    ASSERT_EQ(p - buffer, 20);
    ASSERT_EQ(std::string(buffer, p - buffer), "18446744073709551615");
  }
}

TEST(LowLevelBufferFillerTest, FillHex) {
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillHex(uint8_t(0x54));
    ASSERT_EQ(filler.pointer() - buffer, 2);
    ASSERT_STREQ(buffer, "54");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillHex(uint16_t(0x2554));
    ASSERT_EQ(filler.pointer() - buffer, 4);
    ASSERT_STREQ(buffer, "2554");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillHex(uint32_t(0xab25cd54));
    ASSERT_EQ(filler.pointer() - buffer, 8);
    ASSERT_STREQ(buffer, "ab25cd54");
  }
  {
    char buffer[64] = {};
    LowLevelBufferFiller filler{buffer};
    filler << FillHex(uint64_t(0xab25cd540123));
    ASSERT_EQ(filler.pointer() - buffer, 16);
    ASSERT_STREQ(buffer, "0000ab25cd540123");
  }
}

} // namespace
} // namespace cbu
