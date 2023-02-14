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

#include "cbu/strings/fixed_length_compare.h"

#include <gtest/gtest.h>

namespace cbu {
namespace {

constexpr std::size_t kMax = 1024;

constexpr std::size_t GenLengths(std::size_t* res) noexcept {
  std::size_t n = 0;
  for (std::size_t l = 0; l <= kMax; l += (l < 128 ? 1 : 1 + l / 32)) {
    if (res) res[n] = l;
    ++n;
  }
  return n;
}

constexpr std::size_t kLengthCount = GenLengths(nullptr);

constexpr std::array<std::size_t, kLengthCount> GenLengths() noexcept {
  std::array<std::size_t, kLengthCount> res;
  GenLengths(res.data());
  return res;
}

constexpr std::array<std::size_t, kLengthCount> kLengths = GenLengths();

template <std::size_t Len>
void RunTest(char (&buffer)[kMax]) {
  for (std::size_t from = 0; from < 128 && from + Len <= kMax; ++from) {
    ASSERT_TRUE(IsAllZero<Len>(buffer + from))
        << "from = " << from << " Len = " << Len;
    for (std::size_t i = 0; i < Len; ++i) {
      buffer[from + i] = 127;
      ASSERT_FALSE(IsAllZero<Len>(buffer + from))
          << "from = " << from << " Len = " << Len << " i = " << i;
      buffer[from + i] = 0;
    }
  }
}

TEST(FixedLengthCompareTest, DefaultTest) {
  char buffer[kMax]{};

  auto test_all = [&]<std::size_t... I>(std::index_sequence<I...>) noexcept {
    ((!HasFatalFailure() ? static_cast<void>(RunTest<kLengths[I]>(buffer))
                         : static_cast<void>(0)),
     ...);
  };

  test_all(std::make_index_sequence<kLengthCount>());
}

}  // namespace
}  // namespace cbu
