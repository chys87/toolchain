/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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
#include <gtest/gtest.h>

namespace cbu::cbu_network {

static_assert(IPv4(1, 2, 3, 4).value(native_endian) == 0x01020304);
static_assert(IPv4(1, 2, 3, 4).a() == 1);
static_assert(IPv4(1, 2, 3, 4).b() == 2);
static_assert(IPv4(1, 2, 3, 4).c() == 3);
static_assert(IPv4(1, 2, 3, 4).d() == 4);
static_assert((IPv4(127, 0, 0, 1) & IPv4(255, 0, 0, 0)) == IPv4(127, 0, 0, 0));

TEST(IPv4Test, ToStringTest) {
  for (int i = 0; i < 256; ++i) {
    IPv4 ip(192, 168, 1, i);
    ASSERT_EQ("192.168.1." + std::to_string(i),
              ip.to_string());
  }
}

TEST(IPv4Test, FromStringTest) {
  auto ip = IPv4::from_string("232.32.2.0");
  EXPECT_TRUE(ip.has_value());
  EXPECT_EQ(232, ip->a());
  EXPECT_EQ(32, ip->b());
  EXPECT_EQ(2, ip->c());
  EXPECT_EQ(0, ip->d());

  EXPECT_FALSE(IPv4::from_string("192.168.00.105").has_value());
  EXPECT_FALSE(IPv4::from_string("192.168.01.105").has_value());

  EXPECT_TRUE(IPv4::from_string("255.255.255.255").has_value());
  EXPECT_FALSE(IPv4::from_string("255.255.256.255").has_value());
  EXPECT_FALSE(IPv4::from_string("255.270.255.255").has_value());

  EXPECT_FALSE(IPv4::from_string("192.168.1.105x").has_value());
  EXPECT_FALSE(IPv4::from_string("192.168.1#105").has_value());
  EXPECT_FALSE(IPv4::from_string(" 192.168.1.105").has_value());
  EXPECT_FALSE(IPv4::from_string("192.168.1..105").has_value());
}


} // namepsace cbu::cbu_network
