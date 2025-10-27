/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include "cbu/strings/encoding.h"

#include <iconv.h>

#include <string_view>

#include <gtest/gtest.h>

#include "cbu/common/defer.h"
#include "cbu/debug/gtest_formatters.h"

namespace cbu {
namespace {

using std::operator""sv;

TEST(Utf8Test, ByteType) {
  EXPECT_EQ(Utf8ByteType::ASCII, utf8_byte_type('\0'));
  EXPECT_EQ(Utf8ByteType::ASCII, utf8_byte_type('\x7f'));
  EXPECT_EQ(Utf8ByteType::LEADING, utf8_byte_type(u8"一"[0]));
  EXPECT_EQ(Utf8ByteType::TRAILING, utf8_byte_type(u8"一"[1]));
  EXPECT_EQ(Utf8ByteType::INVALID, utf8_byte_type('\xff'));
}

TEST(Utf8Test, Char32ToUtf8Length) {
  EXPECT_EQ(1, char32_to_utf8_length(U'\u007f'));
  EXPECT_EQ(sizeof(u8"一") - 1, char32_to_utf8_length(U'一'));
  EXPECT_EQ(0, char32_to_utf8_length(char32_t(0x11ffff)));
}

TEST(Utf8Test, Utf8LeadingByteToTrailingLength) {
  EXPECT_EQ(sizeof(u8"\u0080") - 2,
            utf8_leading_byte_to_trailing_length(u8"\u0080"[0]));
  EXPECT_EQ(sizeof(u8"\uFFFF") - 2,
            utf8_leading_byte_to_trailing_length(u8"\uFFFF"[0]));
  EXPECT_EQ(sizeof(u8"\U0010FFFF") - 2,
            utf8_leading_byte_to_trailing_length(u8"\U0010FFFF"[0]));
}

TEST(Utf8Test, Char32ToUtf8) {
  char buffer[16];

  // Unsupported code points
  EXPECT_EQ(char32_to_utf8(buffer, char32_t(0x110000)), nullptr);
  EXPECT_EQ(char32_to_utf8(buffer, char32_t(0xd801)), nullptr);

  EXPECT_EQ(char32_to_utf8(buffer, U'x') - buffer, 1);
  EXPECT_EQ(std::string(buffer, 1), std::string((const char*)u8"x"));

  EXPECT_EQ(char32_to_utf8_unsafe(buffer, U'x') - buffer, 1);
  EXPECT_EQ(std::string(buffer, 1), std::string((const char*)u8"x"));

  EXPECT_EQ(char16_to_utf8_unsafe(buffer, u'x') - buffer, 1);
  EXPECT_EQ(std::string(buffer, 1), std::string((const char*)u8"x"));

  EXPECT_EQ(char32_to_utf8(buffer, U'\u07ff') - buffer, 2);
  EXPECT_EQ(std::string(buffer, 2), std::string((const char*)u8"\u07ff"));

  EXPECT_EQ(char32_to_utf8_unsafe(buffer, U'\u07ff') - buffer, 2);
  EXPECT_EQ(std::string(buffer, 2), std::string((const char*)u8"\u07ff"));

  EXPECT_EQ(char16_to_utf8_unsafe(buffer, u'\u07ff') - buffer, 2);
  EXPECT_EQ(std::string(buffer, 2), std::string((const char*)u8"\u07ff"));

  EXPECT_EQ(char32_to_utf8(buffer, U'人') - buffer, 3);
  EXPECT_EQ(std::string(buffer, 3), std::string((const char*)u8"人"));

  EXPECT_EQ(char32_to_utf8_unsafe(buffer, U'人') - buffer, 3);
  EXPECT_EQ(std::string(buffer, 3), std::string((const char*)u8"人"));

  EXPECT_EQ(char16_to_utf8_unsafe(buffer, u'人') - buffer, 3);
  EXPECT_EQ(std::string(buffer, 3), std::string((const char*)u8"人"));

  EXPECT_EQ(char32_to_utf8(buffer, U'\U00012345') - buffer, 4);
  EXPECT_EQ(std::string(buffer, 4), std::string((const char*)u8"\U00012345"));

  EXPECT_EQ(char32_to_utf8_unsafe(buffer, U'\U00012345') - buffer, 4);
  EXPECT_EQ(std::string(buffer, 4), std::string((const char*)u8"\U00012345"));
}

TEST(Utf8Test, Utf16SurrogatesTest) {
  EXPECT_EQ(char32_non_bmp_to_utf16_surrogates(U'🀄'),
            std::pair(u"🀄"[0], u"🀄"[1]));
  EXPECT_EQ(utf16_surrogates_to_char32(u"🀄"[0], u"🀄"[1]),
            U'🀄');
  {
    char8_t buffer[4];
    utf16_surrogates_to_utf8(buffer, u"🀄"[0], u"🀄"[1]);
    EXPECT_EQ(std::u8string_view(buffer, 4), u8"🀄"sv);
  }

  for (uint32_t u = 0x10000; u <= 0x10ffff; ++u) {
    char32_t c{u};
    char16_t u16[2]{char32_non_bmp_to_utf16_surrogates(c).first,
                    char32_non_bmp_to_utf16_surrogates(c).second};
    EXPECT_EQ(utf16_surrogates_to_char32(u16[0], u16[1]), c);
    char buffer[4]{};
    utf16_surrogates_to_utf8(buffer, u16[0], u16[1]);
    char buffer2[4]{};
    char32_to_utf8(buffer2, c);
    EXPECT_EQ(std::string_view(buffer, 4), std::string_view(buffer2, 4));
  }
}

TEST(Utf8Test, ValidateUtf8) {
  // Sometimes we add a long string so that the AVX version will be used
  const std::string kLong{64, 'x'};

  EXPECT_TRUE(validate_utf8((const char*)u8"x\u00a2\u20ac\U00010348"));
  EXPECT_TRUE(validate_utf8((const char*)u8"x\u00a2\u20ac\U00010348" + kLong));
  EXPECT_FALSE(validate_utf8("\x81\x81\x81"));
  EXPECT_FALSE(validate_utf8("\x81\x81\x81" + kLong));
  EXPECT_FALSE(validate_utf8("\xe0\x83\xb0"));  // Over-long
  EXPECT_FALSE(validate_utf8("\xe0\x83\xb0" + kLong));  // Over-long
  EXPECT_TRUE(
      validate_utf8(u8"0123456789abcdef0123456789abcdef!@#$#@!#$#@!$#@!$#!@$!@$"
                    u8"!@#$!@#$!@#$!@#$#@!$@#!$!@#$!@#$!@#$!@#$!@#$!@#$!@#$!@#$"
                    u8"!@#$@#!$!@#$!@#$!@#$\U00010348"));
  EXPECT_FALSE(validate_utf8(
      "0123456789abcdef0123456789abcdef!@#$#@!#$#@!$#@!$#!@$!@$!@#$!@#$!@#$!@#$"
      "#@!$@#!$!@#$!@#$!@#$!@#$!@#$!@#$!@#$!@#$!@#$@#!$!@#$!@#$!@#$\xff!"));

  // Enumerate all UTF-8 code points
  for (uint32_t c = 0; c < 0x110000; ++c) {
    // Skip UTF-16 surrogate
    if (is_in_utf16_surrogate_range(c)) {
      c = 0xdfff;
      continue;
    }
    char buf[16];
    char* p = char32_to_utf8(buf, char32_t(c));
    ASSERT_NE(p, nullptr);
    size_t l = p - buf;
    ASSERT_TRUE(validate_utf8(buf, l)) << std::string_view(buf, l);
    ASSERT_TRUE(validate_utf8(std::string(buf, l) + kLong))
        << std::string_view(buf, l);
  }

  // Starting from a valid string, and then add random noise.
  iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
  CBU_DEFER { iconv_close(cd); };
  auto naive_validate = [cd](std::string_view sv) {
    char buffer[65536];
    char* inbuf = const_cast<char*>(sv.data());
    size_t inbytes = sv.size();
    char* outbuf = buffer;
    size_t outbytes = sizeof(buffer);
    return iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes) == 0 && inbytes == 0;
  };

#define REPEAT(a) a a a
  constexpr std::u8string_view kStart = REPEAT(
      u8"\uabcd\u1234\0\1\2\3\4\5\6\7\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
      u8"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
      u8"~!@#$%^&*()_"
      u8"\u007f\u0080\u0081\u0091\u00a0\u00c0\u00ed\u00ff\u0100\u0200\u0300"
      u8"\u0abc\u1384\u2554\u2abc\u3a02\u57cf\u8ff3\ua2a7\ucfff\ud7ff\ue000"
      u8"\ueabc\uff2a\uffff"
      u8"\U00010000\U00013784\U0001ABEE\U0002052A\U0003A2A7\U000A7A8B"
      u8"\U000FFFFF\U00101234\U0010FFFF") u8""sv;
#undef REPEAT
  constexpr size_t kL = kStart.size();
  ASSERT_TRUE(naive_validate(
      {reinterpret_cast<const char*>(kStart.data()), kStart.size()}));
  ASSERT_TRUE(validate_utf8(kStart));

  for (size_t i = 0; i < kL; ++i) {
    for (size_t j = i + 1; j < kL && j < i + 4; ++j) {
      char buffer[kL];
      memcpy(buffer, kStart.data(), kL);

      for (int k = 0; k < 256; k += (i < kL / 4) ? 1 : 2) {
        buffer[i] += (i < kL / 4) ? 1 : 2;
        for (int l = 0; l < 256; l += 32) {
          buffer[j] += 32;
          ASSERT_EQ(validate_utf8({buffer, kL}), naive_validate({buffer, kL}))
              << std::string_view(buffer, kL);
        }
      }
    }
  }
}

TEST(Utf16Test, Char32ToUtf16) {
  char16_t buffer[1024];

  {
    auto s = U"abcéê一二三🌍🇨🇳"sv;
    char16_t* r = char32_to_utf16_unsafe(buffer, s.data(), s.size());
    ASSERT_EQ(std::u16string_view(buffer, r), u"abcéê一二三🌍🇨🇳"sv);
  }
}

}  // namespace
} // namespace cbu
