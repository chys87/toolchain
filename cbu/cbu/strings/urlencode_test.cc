/*
 * cbu - chys's basic utilities
 * Copyright (c) 2026, chys <admin@CHYS.INFO>
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

#include "urlencode.h"

#include <cstring>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace cbu {
namespace {

std::string decode(std::string_view s) {
  std::string buffer(s);
  char* end = decode_uri_component(buffer.data(), s);
  return std::string(buffer.data(), end - buffer.data());
}

TEST(UrlencodeTest, Decode) {
  EXPECT_EQ(decode(""), "");
  EXPECT_EQ(decode("abc"), "abc");
  EXPECT_EQ(decode("%41%42%43"), "ABC");
  EXPECT_EQ(decode("%e4%bd%a0%e5%a5%bd"), "\xe4\xbd\xa0\xe5\xa5\xbd");
  // Consecutive escapes (percent-encoded UTF-8): decoded without the SIMD
  // search round trip in between
  EXPECT_EQ(decode("%E4%BD%A0%E5%A5%BD%E4%B8%96%E7%95%8C"),
            "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c");
  EXPECT_EQ(decode("/%E4%BD%A0/%E5%A5%BD/"), "/\xe4\xbd\xa0/\xe5\xa5\xbd/");

  // Unlike decodeURI, reserved characters are decoded as well
  EXPECT_EQ(decode("a%2Fb%3Fc%3Dd%26e"), "a/b?c=d&e");

  // '+' is not treated as a space
  EXPECT_EQ(decode("a+b"), "a+b");

  // '%' not followed by two hexadecimal digits: copied verbatim
  EXPECT_EQ(decode("100%"), "100%");
  EXPECT_EQ(decode("%A"), "%A");
  EXPECT_EQ(decode("%G1"), "%G1");
  EXPECT_EQ(decode("%1G"), "%1G");
  EXPECT_EQ(decode("%%%41"), "%%A");

  // Decoded bytes are not validated as UTF-8
  EXPECT_EQ(decode("%E4%BD"), "\xe4\xbd");
  EXPECT_EQ(decode("%ED%A0%80"), "\xed\xa0\x80");
  EXPECT_EQ(decode("%C0%AF"), "\xc0\xaf");
  EXPECT_EQ(decode(std::string_view("\xff", 1)), "\xff");
}

TEST(UrlencodeTest, NullPointerEmpty) {
  char buffer;
  EXPECT_EQ(decode_uri_component(&buffer, nullptr, 0), &buffer);
}

TEST(UrlencodeTest, Simd) {
  // No '%': exercises the block-copy fast path
  EXPECT_EQ(decode(std::string(100, 'x')), std::string(100, 'x'));

  // '%' at every offset, with src and dst at every alignment
  alignas(32) char sbuf[160];
  alignas(32) char dbuf[160];
  for (unsigned src_off = 0; src_off < 32; ++src_off) {
    for (unsigned dst_off = 0; dst_off < 32; ++dst_off) {
      for (unsigned pos = 0; pos < 48; ++pos) {
        std::string src = std::string(pos, 'a') + "%41" + std::string(48, 'b') +
                          "%zz" + std::string(16, 'c');
        std::string expected = std::string(pos, 'a') + "A" +
                               std::string(48, 'b') + "%zz" +
                               std::string(16, 'c');
        memcpy(sbuf + src_off, src.data(), src.size());
        char* end =
            decode_uri_component(dbuf + dst_off, sbuf + src_off, src.size());
        ASSERT_EQ(std::string_view(dbuf + dst_off, end - (dbuf + dst_off)),
                  expected)
            << "src_off=" << src_off << " dst_off=" << dst_off
            << " pos=" << pos;
      }
    }
  }
}

}  // namespace
}  // namespace cbu
