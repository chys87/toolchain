/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2022, chys <admin@CHYS.INFO>
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

#include "cbu/network/ipv4.h"

#include <arpa/inet.h>

#include <gtest/gtest.h>

#include "cbu/common/short_string.h"
#include "cbu/debug/gtest_formatters.h"

namespace cbu {

using std::operator""sv;

static_assert(IPv4(1, 2, 3, 4).value(native_endian) == 0x01020304);
static_assert(IPv4(1, 2, 3, 4).a() == 1);
static_assert(IPv4(1, 2, 3, 4).b() == 2);
static_assert(IPv4(1, 2, 3, 4).c() == 3);
static_assert(IPv4(1, 2, 3, 4).d() == 4);
static_assert((IPv4(127, 0, 0, 1) & IPv4(255, 0, 0, 0)) == IPv4(127, 0, 0, 0));

TEST(IPv4Test, ToStringTest) {
  for (int i = 0; i < 256; ++i) {
    IPv4 ip(192, 168, 1, i);
    ASSERT_EQ("192.168.1." + std::to_string(i), ip.ToString().string());
  }
}

TEST(IPv4Test, FromCommonStringTest) {
  auto ip = IPv4::FromCommonString("232.32.2.0");
  EXPECT_TRUE(ip.has_value());
  EXPECT_EQ(232, ip->a());
  EXPECT_EQ(32, ip->b());
  EXPECT_EQ(2, ip->c());
  EXPECT_EQ(0, ip->d());

  EXPECT_FALSE(IPv4::FromCommonString("192.168.00.105").has_value());
  EXPECT_FALSE(IPv4::FromCommonString("192.168.01.105").has_value());

  EXPECT_TRUE(IPv4::FromCommonString("255.255.255.255").has_value());
  EXPECT_FALSE(IPv4::FromCommonString("255.255.256.255").has_value());
  EXPECT_FALSE(IPv4::FromCommonString("255.270.255.255").has_value());

  EXPECT_FALSE(IPv4::FromCommonString("192.168.1.105x").has_value());
  EXPECT_FALSE(IPv4::FromCommonString("192.168.1#105").has_value());
  EXPECT_FALSE(IPv4::FromCommonString(" 192.168.1.105").has_value());
  EXPECT_FALSE(IPv4::FromCommonString("192.168.1..105").has_value());
}

TEST(IPv4Test, FromStringTest) {
  EXPECT_EQ(IPv4::FromString("192.168.25.54"), IPv4(0xc0a81936));
  EXPECT_EQ(IPv4::FromString("192.168.6454"), IPv4(0xc0a81936));
  EXPECT_EQ(IPv4::FromString("192.11016502"), IPv4(0xc0a81936));
  EXPECT_EQ(IPv4::FromString("3232241974"), IPv4(0xc0a81936));

  // Overflow test
  EXPECT_EQ(IPv4::FromString("4294967295"), IPv4(0xffffffff));
  EXPECT_EQ(IPv4::FromString("4294967296"), std::nullopt);
  EXPECT_EQ(IPv4::FromString("192.16777215"), IPv4(0xc0ffffff));
  EXPECT_EQ(IPv4::FromString("192.16777216"), std::nullopt);
  EXPECT_EQ(IPv4::FromString("192.168.65535"), IPv4(0xc0a8ffff));
  EXPECT_EQ(IPv4::FromString("192.168.65536"), std::nullopt);
  EXPECT_EQ(IPv4::FromString("192.168.25.255"), IPv4(0xc0a819ff));
  EXPECT_EQ(IPv4::FromString("192.168.25.256"), std::nullopt);
  EXPECT_EQ(IPv4::FromString("192.168.255.54"), IPv4(0xc0a8ff36));
  EXPECT_EQ(IPv4::FromString("192.168.256.54"), std::nullopt);

  // Octal / hexadecimal
  EXPECT_EQ(IPv4::FromString("0x7f000001"), IPv4(0x7f000001));
  EXPECT_EQ(IPv4::FromString("0x7f.00410"), IPv4(0x7f000108));
  EXPECT_EQ(IPv4::FromString("127.0.1"), IPv4(0x7f000001));
  EXPECT_EQ(IPv4::FromString("0177.0.1"), IPv4(0x7f000001));
  EXPECT_FALSE(IPv4::FromString("0x1ffffffff").has_value());
  EXPECT_TRUE(IPv4::FromString("037777777777").has_value());
  EXPECT_FALSE(IPv4::FromString("040000000000").has_value());

  // Bad formats
  EXPECT_FALSE(IPv4::FromString("192.168.1.105x").has_value());
  EXPECT_FALSE(IPv4::FromString("192.168.1#105").has_value());
  EXPECT_FALSE(IPv4::FromString(" 192.168.1.105").has_value());
  EXPECT_FALSE(IPv4::FromString("192.168.1..105").has_value());
  EXPECT_FALSE(IPv4::FromString("0xx7f.0.0.1").has_value());
}

TEST(IPv4Test, InAddrTest) {
  in_addr addr;
  ASSERT_NE(inet_pton(AF_INET, "192.168.5.4", &addr), 0);
  IPv4 ip(addr);
  ASSERT_EQ(std::string_view(ip.ToString()), "192.168.5.4"sv);

  ip.To(&addr);
  char buffer[32];
  ASSERT_NE(inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)), nullptr);
  ASSERT_STREQ(buffer, "192.168.5.4");
}

}  // namespace cbu
