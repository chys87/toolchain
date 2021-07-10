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

#include "cbu/common/unit_prefix.h"

#include <gtest/gtest.h>

#include "cbu/common/fastarith.h"

namespace cbu {

TEST(UnitPrefixTest, Base1000Test) {
  static_assert(split_unit_prefix_1000(0).quot == 0);
  static_assert(split_unit_prefix_1000(0).rem == 0);
  static_assert(split_unit_prefix_1000(0).prefix_idx == 0);
  static_assert(split_unit_prefix_1000(999).quot == 999);
  static_assert(split_unit_prefix_1000(999).rem == 0);
  static_assert(split_unit_prefix_1000(999).prefix_idx == 0);
  static_assert(split_unit_prefix_1000(1000).quot == 1);
  static_assert(split_unit_prefix_1000(1000).rem == 0);
  static_assert(split_unit_prefix_1000(1000).prefix_idx == 1);
  static_assert(split_unit_prefix_1000(999999).quot == 999);
  static_assert(split_unit_prefix_1000(999999).rem == 999);
  static_assert(split_unit_prefix_1000(999999).prefix_idx == 1);
  static_assert(split_unit_prefix_1000(1000000).quot == 1);
  static_assert(split_unit_prefix_1000(1000000).rem == 0);
  static_assert(split_unit_prefix_1000(1000000).prefix_idx == 2);

  for (std::uint64_t v = 0;
       v < std::numeric_limits<std::uint64_t>::max() / 8 * 7; v += 1 + v / 17) {
    auto [div, rem, idx] = split_unit_prefix_1000(v);
    std::uint64_t idx_base = fast_powu(std::uint64_t(1000), idx);
    if (v != 0) {
      ASSERT_NE(div, 0) << "v=" << v;
    }
    ASSERT_EQ(div, v / idx_base) << "v=" << v;
    if (idx == 0) {
      ASSERT_EQ(rem, 0) << "v=" << v;
    } else {
      ASSERT_EQ(rem, v % idx_base / fast_powu(std::uint64_t(1000), idx - 1))
          << "v=" << v;
    }
  }
}

TEST(UnitPrefixTest, Base1024Test) {
  static_assert(split_unit_prefix_1024(0).quot == 0);
  static_assert(split_unit_prefix_1024(0).rem == 0);
  static_assert(split_unit_prefix_1024(0).prefix_idx == 0);
  static_assert(split_unit_prefix_1024(1023).quot == 1023);
  static_assert(split_unit_prefix_1024(1023).rem == 0);
  static_assert(split_unit_prefix_1024(1023).prefix_idx == 0);
  static_assert(split_unit_prefix_1024(1024).quot == 1);
  static_assert(split_unit_prefix_1024(1024).rem == 0);
  static_assert(split_unit_prefix_1024(1024).prefix_idx == 1);
  static_assert(split_unit_prefix_1024(1024 * 1024 - 1).quot == 1023);
  static_assert(split_unit_prefix_1024(1024 * 1024 - 1).rem == 1023);
  static_assert(split_unit_prefix_1024(1024 * 1024 - 1).prefix_idx == 1);
  static_assert(split_unit_prefix_1024(1024 * 1024).quot == 1);
  static_assert(split_unit_prefix_1024(1024 * 1024).rem == 0);
  static_assert(split_unit_prefix_1024(1024 * 1024).prefix_idx == 2);

  for (std::uint64_t v = 0;
       v < std::numeric_limits<std::uint64_t>::max() / 8 * 7; v += 1 + v / 17) {
    auto [div, rem, idx] = split_unit_prefix_1024(v);
    std::uint64_t idx_base = fast_powu(std::uint64_t(1024), idx);
    if (v != 0) {
      ASSERT_NE(div, 0) << "v=" << v;
    }
    ASSERT_EQ(div, v / idx_base) << "v=" << v;
    if (idx == 0) {
      ASSERT_EQ(rem, 0) << "v=" << v;
    } else {
      ASSERT_EQ(rem, v % idx_base / fast_powu(std::uint64_t(1024), idx - 1))
          << "v=" << v;
    }
  }
}

}  // namespace cbu
