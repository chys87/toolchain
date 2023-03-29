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

#include "cbu/common/heapq.h"
#include <gtest/gtest.h>

namespace cbu {
namespace {

std::string Show(const int* array, std::size_t n) {
  std::string r;
  for (std::size_t i = 0; i < n; ++i) {
    r += std::to_string(array[i]);
    r += ' ';
  }
  if (!r.empty()) r.pop_back();
  return r;
}

TEST(HeapTest, HeapTest) {
  {
    int a[]{1, 3, 5, 7, 9, 2, 4, 6, 8, 10};
    heap_make(a, std::size(a));
    ASSERT_TRUE(heap_verify(a, std::size(a))) << Show(a, std::size(a));
  }
  {
    int a[32];
    int n = 0;
    for (int i = 0; i < 16; ++i) heap_push(a, n++, i);
    ASSERT_TRUE(heap_verify(a, 16)) << Show(a, 16);

    for (int i = 0; i < 16; ++i) {
      int v = a[0];
      ASSERT_EQ(v, i);
      heap_pop(a, 16 - i);
      ASSERT_TRUE(heap_verify(a, 16 - i - 1)) << Show(a, 16 - i - 1);
    }
  }
}

}  // namespace
}  // namespace cbu
