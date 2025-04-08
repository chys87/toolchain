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

#include "cbu/strings/short_string.h"

#include <gtest/gtest.h>

#include "cbu/strings/strcomm.h"

namespace cbu {
namespace {

TEST(ShortStringTest, SpecializedEqTest) {
  using S = short_nzstring<31>;
  S a("abc");
  S b("abc");
  ASSERT_EQ(a, b);
  b.assign("abcd");
  ASSERT_NE(a, b);
  b.assign("abd");
  ASSERT_NE(a, b);
  b.assign("abc");
  ASSERT_EQ(a, b);
}

TEST(ShortStringTest, SpecializedStrCmpLengthFirstTest) {
  using S = short_nzstring<31>;
  S a("abc");
  {
    auto& buffer = a.buffer();
    reinterpret_cast<volatile char*>(buffer)[0] = 'a';
    volatile uint8_t l = 3;
    a.set_length(l);
  }

  S b("abc");
  ASSERT_EQ(strcmp_length_first(a, b), 0);
  b.assign("xy");
  ASSERT_GT(strcmp_length_first(a, b), 0);
  b.assign("aaaa");
  ASSERT_LT(strcmp_length_first(a, b), 0);
  b.assign("axy");
  ASSERT_LT(strcmp_length_first(a, b), 0);
  b.assign("aaa");
  ASSERT_GT(strcmp_length_first(a, b), 0);
  b.assign("abc");
  ASSERT_EQ(strcmp_length_first(a, b), 0);
}

}  // namespace
}  // namespace cbu
