/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022, chys <admin@CHYS.INFO>
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

TEST(IPv6Test, FormatTest) {
  auto test = [](const char* src, std::string_view expected) {
    SCOPED_TRACE(src);
    in6_addr addr;
    ASSERT_NE(inet_pton(AF_INET6, src, &addr), 0);
    EXPECT_EQ(std::string_view(IPv6::Format(addr)), expected);
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
  test("0:0::ffff:0:192.168.1.1", "::ffff:0:192.168.1.1");
  test("0:1::ffff:192.168.1.1", "0:1::ffff:c0a8:101");
  test("64:ff9b::", "64:ff9b::0.0.0.0");
  test("64:ff9b:0::2554:909:90a", "64:ff9b::2554:909:90a");
  test("64:ff9b:1::2554:909:90a", "64:ff9b:1::2554:9.9.9.10");
  test("0064:ff9b:0001:ffff:ffff:ffff:255.255.255.255",
       "64:ff9b:1:ffff:ffff:ffff:255.255.255.255");
}

}  // namespace
}  // namespace cbu
