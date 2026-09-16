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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS IN A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cbu/strings/fixed_string.h"

#include <string_view>

#include <gtest/gtest.h>

namespace cbu {
namespace {

TEST(FixedStringTest, OperatorPlus) {
  constexpr auto a = basic_fixed_string<2>("ab");
  constexpr auto b = basic_fixed_string<3>("cde");
  constexpr auto c = a + b;
  static_assert(c.size() == 5);
  static_assert(std::string_view(c) == "abcde");
}

TEST(FixedStringTest, OperatorPlusNonChar) {
  // Used to fail to compile: the local variable in operator+ missed the
  // character type template argument
  constexpr auto wa = basic_fixed_string<2, false, wchar_t>(L"ab");
  constexpr auto wb = basic_fixed_string<3, false, wchar_t>(L"cde");
  constexpr auto wc = wa + wb;
  static_assert(wc.size() == 5);
  static_assert(std::wstring_view(wc) == std::wstring_view(L"abcde", 5));

  constexpr auto ua = basic_fixed_string<2, false, char16_t>(u"ab");
  constexpr auto ub = basic_fixed_string<3, false, char16_t>(u"cde");
  constexpr auto uc = ua + ub;
  static_assert(uc.size() == 5);
  static_assert(std::u16string_view(uc) ==
                std::u16string_view(u"abcde", 5));
}

}  // namespace
}  // namespace cbu
