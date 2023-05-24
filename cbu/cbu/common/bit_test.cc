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

#include "bit.h"
#include <gtest/gtest.h>

namespace cbu {

TEST(Bittest, RbitTest) {
  {
    constexpr auto x = rbit(std::uint8_t(0b10101110));
    ASSERT_EQ(x, 0b01110101);
  }
  {
    constexpr auto x = rbit(std::uint16_t(0x0133));
    ASSERT_EQ(x, 0xcc80);
  }
  {
    constexpr auto x = rbit(std::uint32_t(0x01234567));
    ASSERT_EQ(x, 0xe6a2c480);
  }
  {
    constexpr auto x = rbit(std::uint64_t(0x0123456789abcdef));
    ASSERT_EQ(x, 0xf7b3d591e6a2c480ull);
  }
  volatile char zero = 0;
  {
    auto x = rbit(std::uint8_t(0b10101110 + zero));
    ASSERT_EQ(x, 0b01110101);
  }
  {
    auto x = rbit(std::uint16_t(0x0133 + zero));
    ASSERT_EQ(x, 0xcc80);
  }
  {
    auto x = rbit(std::uint32_t(0x01234567 + zero));
    ASSERT_EQ(x, 0xe6a2c480);
  }
  {
    auto x = rbit(std::uint64_t(0x0123456789abcdef + zero));
    ASSERT_EQ(x, 0xf7b3d591e6a2c480ull);
  }
}

TEST(BitTest, SetBits) {
  std::vector<unsigned> vec;
  for (unsigned k : set_bits(0x10012)) {
    vec.push_back(k);
  }
  EXPECT_EQ(std::vector<unsigned>({1, 4, 16}), vec);
}

} // namespace cbu
