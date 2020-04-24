/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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

#include "encoding.h"
#include <gtest/gtest.h>

namespace cbu {

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

} // namespace cbu
