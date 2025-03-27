/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2025, chys <admin@CHYS.INFO>
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

#include "cbu/strings/str_builder.h"

#include <gtest/gtest.h>

#include "cbu/strings/low_level_buffer_filler.h"

namespace cbu::sb {
namespace {

using std::operator""sv;

TEST(StringBuilderTest, BasicTest) {
  ASSERT_EQ(Concat("abc", "def", Optional(true, "a"), Optional(false, "b"))
                .as_string(),
            "abcdefa");
  ASSERT_EQ(Concat(FillDec(25), FillDec<10000, FillOptions{.width = 3}>(54))
                .as_string(),
            "25054");
  ASSERT_EQ(Conditional(true, 'a', 'b').as_string(), "a");
  ASSERT_EQ(Conditional(false, 'a', 'b').as_string(), "b");
  ASSERT_EQ(Conditional(true, 'a', "bbb").as_string(), "a");
  ASSERT_EQ(Conditional(false, 'a', "bbb").as_string(), "bbb");
}

TEST(StringBuilderTest, ConstexprTest) {
  constexpr auto str = []() constexpr noexcept {
    constexpr auto builder = Concat("Hello", FillDec(1234567));
    std::array<char, builder.size()> r;
    builder.write(r.data());
    return r;
  }();
  static_assert(std::string_view(str.data(), str.size()) == "Hello1234567"sv);
}

}  // namespace
}  // namespace cbu::sb
