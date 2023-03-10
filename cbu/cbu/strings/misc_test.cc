/*
 * cbu - chys's basic utilities
 * Copyright (c) 2023, chys <admin@CHYS.INFO>
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

#include "cbu/strings/misc.h"

#include <stdlib.h>

#include <gtest/gtest.h>

namespace cbu {
namespace {

char* dos2unix_naive(std::string_view data, char* dst) noexcept {
  for (char c : data) {
    if (c != '\r') *dst++ = c;
  }
  return dst;
}

TEST(MiscTest, Dos2UnixTest) {
  constexpr size_t kN = 8192;
  char src[kN];
  char dst1[kN];
  char dst2[kN];

  srand(0);
  for (char& c : src) c = rand() & 31;

  for (size_t start = 0; start < 35; ++start) {
    for (size_t end = start; end < kN; end += 1 + (end > 35 ? end / 4 : 0)) {
      std::string_view src_sv{src + start, end - start};
      size_t l1 = dos2unix(src_sv, dst1) - dst1;
      size_t l2 = dos2unix_naive(src_sv, dst2) - dst2;
      ASSERT_EQ(l1, l2) << "start = " << start << " end = " << end
                        << " len = " << end - start;
      ASSERT_EQ(std::string_view(dst1, l1), std::string_view(dst2, l2))
          << "start = " << start << " end = " << end
          << " len = " << end - start;
    }
  }
}

}  // namespace
}  // namespace cbu
