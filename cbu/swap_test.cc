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

#include "swap.h"
#include <gtest/gtest.h>

namespace cbu {

TEST(SwapTest, Default) {
  int a = 2;
  int b = 3;

  swap(a, b);
  EXPECT_EQ(3, a);
  EXPECT_EQ(2, b);
}

TEST(SwapTest, Member) {

  struct Obj {
    int x;
    int swaps = 0;
    Obj(int X) : x(X) {}
    void swap(Obj &other) { cbu::swap(x, other.x); swaps++; other.swaps++; }
  };

  Obj a(2);
  Obj b(3);

  EXPECT_EQ(0, a.swaps);
  EXPECT_EQ(0, b.swaps);
  swap(a, b);
  EXPECT_EQ(3, a.x);
  EXPECT_EQ(2, b.x);
  EXPECT_EQ(1, a.swaps);
  EXPECT_EQ(1, b.swaps);

}

TEST(SwapTest, Array) {
  int a[2] {1, 2};
  int b[2] {3, 4};
  swap(a, b);
  EXPECT_EQ(3, a[0]);
  EXPECT_EQ(4, a[1]);
  EXPECT_EQ(1, b[0]);
  EXPECT_EQ(2, b[1]);
}

} // namespace cbu
