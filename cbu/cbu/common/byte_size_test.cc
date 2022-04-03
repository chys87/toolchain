/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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

#include "byte_size.h"

#include <gtest/gtest.h>

namespace cbu {

TEST(ByteSizeTest, ByteDistanceTest) {
  char x[5];
  EXPECT_EQ(4, byte_distance(x, x + 4));
  EXPECT_EQ(4, byte_distance(x, static_cast<const char*>(x + 4)));
  EXPECT_EQ(x + 4, byte_advance(x, 4));

  int y[10];
  EXPECT_EQ(10 * sizeof(int),
            byte_distance(static_cast<const int*>(y), y + 10));
  EXPECT_EQ(y + 4, byte_advance(y, 4 * sizeof(int)));
  EXPECT_EQ(y, byte_advance(y + 4, -4 * sizeof(int)));
}

TEST(ByteSizeTest, ByteSizeTypeTest) {
  int x[5];

  ByteSize<int> size(x, x + 5);
  EXPECT_EQ(5, std::size_t(size));
  EXPECT_TRUE(bool(size));
  EXPECT_EQ(5, size.size());
  EXPECT_EQ(5 * sizeof(int), size.bytes());

  ++size;
  EXPECT_EQ(6 * sizeof(int), size.bytes());
  EXPECT_EQ(12 * sizeof(int), (size * 2).bytes());
}

TEST(ByteSizeTest, TypeConversionTest) {
  EXPECT_EQ(ByteSize<char[3]>(ByteSize<char[5]>(7)), 7);
  EXPECT_EQ(ByteSize<char[3]>(ByteSize<char[6]>(7)), 7);
  EXPECT_EQ(ByteSize<char[6]>(ByteSize<char[3]>(7)), 7);
}

TEST(ByteSizeTest, PointerOperationsTest) {
  int x[128];

  EXPECT_EQ(&x[5], &x[0] + ByteSize<int>(5));
  EXPECT_EQ(&x[5], ByteSize<int>(5) + &x[0]);

  EXPECT_EQ(&x[2], &x[5] - ByteSize<int>(3));

}

}  // namespace cbu
