/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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

#include "fastdiv.h"
#include <gtest/gtest.h>

namespace cbu {

TEST(FastDiv, Magics) {
  auto p = fastdiv_detail::magic<uint32_t>(3, 31);
  EXPECT_EQ(0, p.S);
  EXPECT_EQ(11, p.M);
  EXPECT_EQ(5, p.N);

  p = fastdiv_detail::magic<uint32_t>(8, 0xffffffff);
  EXPECT_EQ(0, p.S);
  EXPECT_EQ(1, p.M);
  EXPECT_EQ(3, p.N);
}

template <uint64_t D, uint64_t MAX>
inline void test_div() {
  for (uint64_t v = 0; v <= MAX; v = v + 1 + v / 100) {
    ASSERT_EQ(v / D, (fastdiv<D, MAX>(v)));
    ASSERT_EQ(v % D, (fastmod<D, MAX>(v)));
  }
}

TEST(FastDiv, Div) {
  test_div<2, 16383>();
  test_div<3, 122>();
  test_div<3, 123456788>();
  test_div<7, 65536>();
  test_div<7, 65535>();
  test_div<7, 16383>();
  test_div<14, 65535>();
  test_div<21, 65535>();
  test_div<13, 0x1111110>();
  test_div<13, 0x110>();
  test_div<2, 0x123456788>();
  test_div<3, 0x123456789abc>();
  test_div<7, 0x123456789abc>();
  test_div<11, 0x123456789abc>();
  test_div<13, 0x123456789abc>();
  test_div<17, 0x123456789abc>();
  test_div<5, 0x1'0000'0000>();
}

} // namespace cbu
