/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022-2023, chys <admin@CHYS.INFO>
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

#include "cbu/network/ipv6.h"

#include <arpa/inet.h>

#include <string_view>

#include <gtest/gtest.h>

#include "cbu/common/short_string.h"
#include "cbu/debug/gtest_formatters.h"

namespace cbu {
namespace {

using std::operator""sv;

TEST(IPv6Test, FormatTest) {
  auto test = [](const char* src, std::string_view expected, int flags = 0) {
    SCOPED_TRACE(src);
    in6_addr addr;
    ASSERT_NE(inet_pton(AF_INET6, src, &addr), 0);
    EXPECT_EQ(std::string_view(IPv6::Format(addr, flags)), expected);
  };
  test("0::0", "::");
  test("0::f", "::f");
  test("0::3f", "::3f");
  test("0::a3f", "::a3f");
  test("0::9a3f", "::9a3f");
  test("ff::0", "ff::");
  test("a:0:b:0:c:0:d:e", "a::b:0:c:0:d:e");
  test("a:0:b:0:0:c:0:e", "a:0:b::c:0:e");
  test("a:0:0:b:0:0:0:e", "a:0:0:b::e");
  test("a:0:0:0:c:0:0:0", "a::c:0:0:0");
  test("0:0:0:a:c:0:0:0", "::a:c:0:0:0");
  test("0:0:0:a:0:0:0:0", "0:0:0:a::");
  test("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
       "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

  test("0:0::ffff:192.168.1.1", "::ffff:192.168.1.1");
  test("0:0::ffff:192.168.1.1", "192.168.1.1", IPv6::kPreferBareIPv4);
  test("0:0::ffff:0:192.168.1.1", "::ffff:0:192.168.1.1");
  test("0:0::ffff:0:192.168.1.1", "::ffff:0:192.168.1.1",
       IPv6::kPreferBareIPv4);
  test("0:1::ffff:192.168.1.1", "0:1::ffff:c0a8:101");
  test("64:ff9b::", "64:ff9b::0.0.0.0");
  test("64:ff9b:0::2554:909:90a", "64:ff9b::2554:909:90a");
  test("64:ff9b:1::2554:909:90a", "64:ff9b:1::2554:9.9.9.10");
  test("0064:ff9b:0001:ffff:ffff:ffff:255.255.255.255",
       "64:ff9b:1:ffff:ffff:ffff:255.255.255.255");
}

TEST(IPv6Test, IPv6PortTest) {
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 79}.ToString()),
      "[::ffff:192.168.25.54]:79"sv);
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 0}.ToString()),
      "[::ffff:192.168.25.54]:0"sv);
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 7}.ToString()),
      "[::ffff:192.168.25.54]:7"sv);
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 179}.ToString()),
      "[::ffff:192.168.25.54]:179"sv);
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 5179}.ToString()),
      "[::ffff:192.168.25.54]:5179"sv);
  EXPECT_EQ(
      std::string_view(IPv6Port{IPv6{IPv4{192, 168, 25, 54}}, 51793}.ToString()),
      "[::ffff:192.168.25.54]:51793"sv);
}

TEST(IPv6Test, FromStringTest) {
  auto test_valid = [](std::string_view src, std::string_view standard = {}) {
    auto ipv6_opt = IPv6::FromString(src);
    ASSERT_TRUE(ipv6_opt.has_value()) << src;
    ASSERT_EQ(std::string_view(ipv6_opt->ToString()),
              standard.empty() ? src : standard);
  };
  auto test_invalid = [](std::string_view src) {
    auto ipv6_opt = IPv6::FromString(src);
    ASSERT_FALSE(ipv6_opt.has_value()) << src;
  };

  test_valid("192.168.1.5", "::ffff:192.168.1.5");
  test_invalid("[192.168.1.5]");
  test_valid("::ffff:192.168.1.5");
  test_valid("[::ffff:192.168.1.5]", "::ffff:192.168.1.5");
  test_invalid("ffff:192.168.1.5");
  test_valid("::1");
  test_valid("::1:abcd");
  test_valid("abcd::");
  test_valid("abcd::1");
  test_valid("abcd::1:dead:beef");
  test_valid("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  test_invalid(":ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  test_invalid("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:");
  test_invalid("0ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  test_invalid("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:abcd");
  test_valid("64:ff9b::", "64:ff9b::0.0.0.0");
  test_valid("64:ff9b::1.2.3.4", "64:ff9b::1.2.3.4");
}

TEST(IPv6Test, LocalhostTest) {
  ASSERT_EQ(IPv6::Localhost().ToString().string_view(), "::1"sv);
  ASSERT_TRUE(IPv6::Localhost().IsIPv6Localhost());
  ASSERT_TRUE(IPv6(IPv4(127, 2, 3, 4)).IsIPv4Localhost());
  ASSERT_FALSE(IPv6(IPv4(127, 2, 3, 4)).IsIPv4CanonicalLocalhost());
  ASSERT_FALSE(IPv6(IPv4(128, 2, 3, 4)).IsIPv4Localhost());
  ASSERT_FALSE(IPv6(IPv4(128, 2, 3, 4)).IsIPv4CanonicalLocalhost());
}

}  // namespace
}  // namespace cbu
