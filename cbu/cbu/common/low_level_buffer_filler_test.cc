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

#include "cbu/common/low_level_buffer_filler.h"

#include <gtest/gtest.h>

namespace cbu {
inline namespace cbu_low_level_buffer_fillter {
namespace {

using std::operator""sv;

TEST(LowLevelBufferFillerTest, NonConstExpr) {
  char buffer[4096];

  LowLevelBufferFiller filler{buffer};
  filler << 'x'
    << "yz"sv
    << FillLittleEndian(uint16_t(258))
    << FillBigEndian(uint16_t(258));

  char* p = filler.pointer();
  auto size = p - buffer;
  EXPECT_EQ(7, size);
  EXPECT_EQ("xyz\x02\x01\x01\x02"sv, std::string_view(buffer, size));
}

struct StaticBuffer {
  char buffer[128];
  size_t n;
};

TEST(LowLevelBufferFillerTest, ConstExpr) {
  constexpr StaticBuffer sb = []() {
    StaticBuffer res;
    LowLevelBufferFiller filler{res.buffer};
    filler < 'x'
      < "yz"sv
      < FillLittleEndian(uint16_t(258))
      < FillBigEndian(uint16_t(258));
    res.n = filler.pointer() - res.buffer;
    return res;
  }();
  static_assert(7 == sb.n);
  EXPECT_EQ("xyz\x02\x01\x01\x02"sv, std::string_view(sb.buffer, sb.n));
}

} // namespace
} // inline namespace cbu_low_level_buffer_fillter
} // namespace cbu
