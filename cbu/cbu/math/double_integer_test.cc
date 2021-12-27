/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

#include "cbu/math/double_integer.h"

#include <gtest/gtest.h>
#include <stdint.h>

namespace cbu {
namespace {

// We define it so that gtests prints more readable messages, but we don't
// define it in the header to avoid including <iostream>
template <typename T>
std::ostream& operator<<(std::ostream& s, const DoubleUnsigned<T>& v) {
  return s << '[' << v.lo << ',' << v.hi << ']';
}

using U8x2 = DoubleUnsigned<uint8_t>;
using U32x2 = DoubleUnsigned<uint32_t>;

TEST(DoubleUnsignedTest, Construction) {
  for (unsigned x = 0; x < 0x10000; ++x) {
    U8x2 u(x);
    ASSERT_EQ(u.lo, uint8_t(x)) << "x=" << x;
    ASSERT_EQ(u.hi, x >> 8) << "x=" << x;
  }
}

// Let x at least cover all powers of 2 and all powers of 2 minus 1
inline uint64_t inc(uint64_t x) {
  if (x < 128)
    return x + 1;

  for (uint32_t shift = 32; shift; shift /= 2) {
    uint32_t y = x | (x >> shift);
    if (y != x)
      return y;
  }
  return x + 1;
}

TEST(DoubleUnsignedTest, FromMul) {
  for (unsigned x = 0; x < 256; ++x) {
    for (unsigned y = 0; y < 256; ++y) {
      ASSERT_EQ(U8x2::from_mul(x, y), U8x2(x * y)) << "x=" << x << "y=" << y;
    }
  }

  for (uint64_t x = 0; x < (uint64_t(1) << 32); x = inc(x)) {
    for (uint64_t y = 0; y < (uint64_t(1) << 32); y = inc(y)) {
      ASSERT_EQ(U32x2::from_mul(x, y), U32x2(x * y)) << "x=" << x << " y=" << y;
    }
  }
}

TEST(DoubleUnsignedTest, AddOverflow) {
  for (uint32_t x = 0; x < 0x10000; x = inc(x)) {
    for (uint32_t y = 0; y < 0x10000; y = inc(y)) {
      uint16_t z;
      bool overflow = add_overflow(x, y, &z);
      U8x2 u(x);
      U8x2 v(y);
      ASSERT_EQ(u.add_overflow(v), overflow) << "x=" << x << " y=" << y;
      ASSERT_EQ(u, U8x2(z));
    }
  }
}

TEST(DoubleUnsignedTest, SubOverflow) {
  for (uint32_t x = 0; x < 0x10000; x = inc(x)) {
    for (uint32_t y = 0; y < 0x10000; y = inc(y)) {
      uint16_t z;
      bool overflow = sub_overflow(x, y, &z);
      U8x2 u(x);
      U8x2 v(y);
      ASSERT_EQ(u.sub_overflow(v), overflow) << "x=" << x << " y=" << y;
      ASSERT_EQ(u, U8x2(z));
    }
  }
}

TEST(DoubleUnsignedTest, MulOverflow) {
  for (uint32_t x = 0; x < 0x10000; x = inc(x)) {
    for (uint32_t y = 0; y < 0x10000; y = inc(y)) {
      uint16_t z;
      bool overflow = mul_overflow(x, y, &z);
      U8x2 u(x);
      U8x2 v(y);
      ASSERT_EQ(u.mul_overflow(v), overflow) << "x=" << x << " y=" << y;
      ASSERT_EQ(u, U8x2(z));
    }
  }
}

TEST(DoubleUnsignedTest, ShlOverflow) {
  for (uint32_t x = 0; x < 0x10000; ++x) {
    for (unsigned shift = 0; shift < 24; ++shift) {
      uint16_t shifted = x << shift;
      bool overflow = (shifted != (uint64_t(x) << shift));

      U8x2 a(x);
      ASSERT_EQ(a.shl_overflow(shift), overflow)
          << "x=" << x << " shift=" << shift;
      ASSERT_EQ(a, U8x2(shifted)) << "x=" << x << " shift=" << shift;
    }
  }
}

TEST(DoubleUnsignedTest, Shr) {
  for (uint32_t x = 0; x < 0x10000; ++x) {
    for (unsigned shift = 0; shift < 24; ++shift) {
      U8x2 a(x);
      a.shr(shift);
      ASSERT_EQ(a, U8x2(x >> shift)) << "x=" << x << " shift=" << shift;
    }
  }
}

}  // namespace
}  // namespace cbu
