
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

#include "cbu/storage/string_pack.h"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace cbu {
namespace {

using std::operator""s;
using std::operator""sv;

TEST(StringPackTest, StringPack) {
  StringPack sp;
  sp << "Hello"
     << "world"
     << "!"
     << "";

  std::string serialized = sp.serialize();
  ASSERT_EQ(serialized, "\x05Hello\x05world\x01!\x00"sv);

  {
    std::vector<std::string_view> vec;
    sp.for_each([&](std::string_view sv) { vec.push_back(sv); });
    ASSERT_EQ(vec, std::vector<std::string_view>(
                       {"Hello"sv, "world"sv, "!"sv, ""sv}));
  }

  ASSERT_TRUE(sp.deserialize(std::move(serialized)));

  {
    std::vector<std::string_view> vec;
    sp.for_each([&](std::string_view sv) { vec.push_back(sv); });
    ASSERT_EQ(vec, std::vector<std::string_view>(
                       {"Hello"sv, "world"sv, "!"sv, ""sv}));
  }
}

TEST(StringPackTest, StringPackZ) {
  StringPackZ sp;
  sp << "Hello "sv.substr(0, 5) << "world"
     << "!"
     << "";

  std::string serialized = sp.serialize();
  ASSERT_EQ(serialized, "\x05Hello\0\x05world\0\x01!\0\0\0"sv);

  {
    std::vector<cbu::zstring_view> vec;
    sp.for_each([&](cbu::zstring_view sv) { vec.push_back(sv); });
    ASSERT_EQ(vec, std::vector<cbu::zstring_view>(
                       {"Hello"_sv, "world"_sv, "!"_sv, ""_sv}));
  }

  ASSERT_TRUE(sp.deserialize(std::move(serialized)));

  {
    std::vector<cbu::zstring_view> vec;
    sp.for_each([&](cbu::zstring_view sv) { vec.push_back(sv); });
    ASSERT_EQ(vec, std::vector<cbu::zstring_view>(
                       {"Hello"_sv, "world"_sv, "!"_sv, ""_sv}));
  }
}

TEST(StringPackTest, Security) {
  EXPECT_FALSE(StringPack().deserialize("\x06Hello"s));
  EXPECT_FALSE(StringPack().deserialize("\xff"s));

  EXPECT_FALSE(StringPackZ().deserialize("\x05Hello\x01"s));
  EXPECT_TRUE(StringPackZ().deserialize("\x05Hello\x00"s));
}

TEST(StringPackTest, CommonPrefixSuffixCodecTest) {
  EXPECT_EQ(CommonPrefixSuffixCodec::common_prefix_max_31("abcd", "abcc"), 3);
  EXPECT_EQ(CommonPrefixSuffixCodec::common_prefix_max_31("abcd", "xbcc"), 0);
  EXPECT_EQ(CommonPrefixSuffixCodec::common_prefix_max_31(
                "012345678901234567890123456789abcd",
                "012345678901234567890123456789abcdefg"),
            31);

  EXPECT_EQ(CommonPrefixSuffixCodec::common_suffix_max_7("abcd", "xbcd"), 3);
  EXPECT_EQ(CommonPrefixSuffixCodec::common_suffix_max_7("abcd", "abc!"), 0);
  EXPECT_EQ(CommonPrefixSuffixCodec::common_suffix_max_7(
                "012345678901234567890123456789abcd",
                "!012345678901234567890123456789abcd"),
            7);
}

TEST(StringPackTest, Compact) {
  {
    StringPackCompactEncoder encoder;
    encoder << "a1.txt" << "a22.txt" << "a23.txt";

    std::string serialized = encoder.serialize();
    ASSERT_EQ(serialized, "\0\6a1.txt\x0c\00222\x14\0013"sv);

    std::vector<std::string> vec;
    ASSERT_TRUE(string_pack_compact_decode(
        serialized, [&](const std::string& s) { vec.push_back(s); }));
    ASSERT_EQ(vec, std::vector<std::string>({"a1.txt", "a22.txt", "a23.txt"}));
  }

  {
    StringPackCompactEncoder encoder;
    encoder << std::string(15, 'a') + std::string(64, '!') + std::string(6, 'b')
            << std::string(17, 'a') + std::string(64, '@') +
                   std::string(9, 'b');

    ASSERT_LT(encoder.serialize().size(), 15 + 64 + 6 + 17 + 64 + 9);

    std::vector<std::string> vec;
    ASSERT_TRUE(string_pack_compact_decode(
        encoder.serialize(), [&](const std::string& s) { vec.push_back(s); }));
    ASSERT_EQ(vec, std::vector<std::string>(
                       {std::string(15, 'a') + std::string(64, '!') +
                            std::string(6, 'b'),
                        std::string(17, 'a') + std::string(64, '@') +
                            std::string(9, 'b')}));
  }

  {
    StringPackCompactEncoder encoder;
    encoder << std::string(64, 'a') << std::string(32, 'a');

    ASSERT_LT(encoder.serialize().size(), 64 + 32);

    std::vector<std::string> vec;
    ASSERT_TRUE(string_pack_compact_decode(
        encoder.serialize(), [&](const std::string& s) { vec.push_back(s); }));
    ASSERT_EQ(vec, std::vector<std::string>(
                       {std::string(64, 'a'), std::string(32, 'a')}));
  }
}

}  // namespace
}  // namespace cbu
