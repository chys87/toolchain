/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include "cbu/common/utility.h"

#include <gtest/gtest.h>

namespace cbu {

TEST(PairTest, Comparisons) {
  pair<int, int> a{5, 6};
  pair<int, int> b{5, 7};
  pair<int, int> c{4, 7};
  EXPECT_EQ(a, a);
  EXPECT_NE(a, b);
  EXPECT_LT(a, b);
  EXPECT_GT(a, c);
}

TEST(ReversedComparatorTest, Compare) {
  std::less cmp;
  ReversedComparator rcmp{cmp};
  ReversedComparator<std::remove_reference_t<decltype(cmp)>> rcmp_copy{rcmp};
  ReversedComparator<std::remove_reference_t<decltype(rcmp)>> rrcmp{rcmp};

  EXPECT_TRUE(cmp(1, 2));
  EXPECT_FALSE(cmp(2, 1));
  EXPECT_FALSE(rcmp(1, 2));
  EXPECT_TRUE(rcmp(2, 1));
  EXPECT_FALSE(rcmp_copy(1, 2));
  EXPECT_TRUE(rcmp_copy(2, 1));
  EXPECT_TRUE(rrcmp(1, 2));
  EXPECT_FALSE(rrcmp(2, 1));
}

} // namespace cbu
