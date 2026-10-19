/*
 * cbu - chys's basic utilities
 * Copyright (c) 2023-2026, chys <admin@CHYS.INFO>
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

#include "cbu/strings/hex.h"

#include <string_view>

#include <gtest/gtest.h>

namespace cbu {
namespace {

using std::operator""sv;

static_assert(convert_4xdigit("abcd") == 0xabcd);
static_assert(convert_8xdigit("01234567") == 0x01234567u);
static_assert(convert_16xdigit("0123456789abcdef") == 0x0123456789abcdefull);
static_assert(
    convert_16xdigit<false, std::endian::native>("0123456789abcdef") ==
    (std::endian::native == std::endian::big ? 0x0123456789abcdefull
                                             : 0xefcdab8967452301ull));
static_assert(convert_2xdigit("4f") == 0x4f);
static_assert(convert_2xdigit<true>("4f") == 0x4f);
static_assert(convert_2xdigit<true>("00") == 0);

TEST(HexTest, ConvertXDigitEndianTest) {
  // Regression test: convert_2xdigit<true> used not to compile
  EXPECT_EQ(convert_2xdigit("4f"), 0x4f);
  EXPECT_EQ(convert_2xdigit<true>("4f"), 0x4f);
  EXPECT_EQ(convert_2xdigit<true>("00"), 0);
  EXPECT_FALSE(convert_2xdigit("g0").has_value());
  EXPECT_FALSE(convert_2xdigit("4g").has_value());

  EXPECT_EQ(convert_4xdigit("abcd"), 0xabcd);
  EXPECT_EQ((convert_4xdigit<false, std::endian::big>("abcd")), 0xabcd);
  EXPECT_EQ((convert_4xdigit<false, std::endian::little>("abcd")), 0xcdab);

  EXPECT_EQ(convert_8xdigit("01234567"), 0x01234567u);
  EXPECT_EQ((convert_8xdigit<false, std::endian::big>("01234567")),
            0x01234567u);
  EXPECT_EQ((convert_8xdigit<false, std::endian::little>("01234567")),
            0x67452301u);

  EXPECT_EQ(convert_16xdigit("0123456789abcdef"), 0x0123456789abcdefull);
  EXPECT_EQ((convert_16xdigit<false, std::endian::big>("0123456789abcdef")),
            0x0123456789abcdefull);
  EXPECT_EQ((convert_16xdigit<false, std::endian::little>("0123456789abcdef")),
            0xefcdab8967452301ull);

  EXPECT_FALSE((convert_4xdigit<false, std::endian::little>("00g0")));
  EXPECT_FALSE(
      (convert_8xdigit<false, std::endian::little>("000g0000")));
  EXPECT_FALSE(
      (convert_16xdigit<false, std::endian::little>(
          "000000000000000g0000000000000000")));
}

TEST(HexTest, ToHexTest) {
  char buf[32];

  ByteToHex(buf, uint8_t(0xab));
  ASSERT_EQ(std::string_view(buf, 2), "ab"sv);
  ToHex(buf, uint8_t(0xab));
  ASSERT_EQ(std::string_view(buf, 2), "ab"sv);

  LittleEndian16ToHex(buf, 0xabcd);
  ASSERT_EQ(std::string_view(buf, 4), "cdab"sv);
  ToHex(buf, uint16_t(0xabcd));
  ASSERT_EQ(std::string_view(buf, 4), "abcd"sv);

  LittleEndian32ToHex(buf, 0x90abcdef);
  ASSERT_EQ(std::string_view(buf, 8), "efcdab90"sv);
  ToHex(buf, 0x90abcdefu);
  ASSERT_EQ(std::string_view(buf, 8), "90abcdef"sv);

  LittleEndian64ToHex(buf, 0x1234567890abcdef);
  ASSERT_EQ(std::string_view(buf, 16), "efcdab9078563412"sv);
  ToHex(buf, uint64_t(0x1234567890abcdef));
  ASSERT_EQ(std::string_view(buf, 16), "1234567890abcdef"sv);
}

TEST(HexTest, ToHexUpperStyleTest) {
  char buf[32];

  ByteToHex<ToHexStyle::UPPER>(buf, uint8_t(0xab));
  ASSERT_EQ(std::string_view(buf, 2), "AB"sv);
  ToHex<ToHexStyle::UPPER>(buf, uint8_t(0xab));
  ASSERT_EQ(std::string_view(buf, 2), "AB"sv);

  LittleEndian16ToHex<ToHexStyle::UPPER>(buf, 0xabcd);
  ASSERT_EQ(std::string_view(buf, 4), "CDAB"sv);
  ToHex<ToHexStyle::UPPER>(buf, uint16_t(0xabcd));
  ASSERT_EQ(std::string_view(buf, 4), "ABCD"sv);

  LittleEndian32ToHex<ToHexStyle::UPPER>(buf, 0x90abcdef);
  ASSERT_EQ(std::string_view(buf, 8), "EFCDAB90"sv);
  ToHex<ToHexStyle::UPPER>(buf, 0x90abcdefu);
  ASSERT_EQ(std::string_view(buf, 8), "90ABCDEF"sv);

  LittleEndian64ToHex<ToHexStyle::UPPER>(buf, 0x1234567890abcdef);
  ASSERT_EQ(std::string_view(buf, 16), "EFCDAB9078563412"sv);
  ToHex<ToHexStyle::UPPER>(buf, uint64_t(0x1234567890abcdef));
  ASSERT_EQ(std::string_view(buf, 16), "1234567890ABCDEF"sv);
}

}  // namespace
}  // namespace cbu
