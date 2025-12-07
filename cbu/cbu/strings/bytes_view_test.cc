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

#include "bytes_view.h"
#if __has_include(<span>)
# include <span>
#endif
#include <string>
#include <string_view>
#include <gtest/gtest.h>


namespace cbu {

using namespace cbu_bytes_view_types;

// is_implicit_type
static_assert(is_implicit_type_v<char>);
static_assert(is_implicit_type_v<unsigned char>);
static_assert(is_implicit_type_v<signed char>);
static_assert(is_implicit_type_v<char8_t>);
static_assert(is_implicit_type_v<std::byte>);
static_assert(!is_implicit_type_v<bool>);
static_assert(!is_implicit_type_v<int>);
enum struct MyByte : unsigned char {};
static_assert(!is_implicit_type_v<MyByte>);

// is_valid_type
static_assert(is_valid_type_v<int>);
static_assert(!is_valid_type_v<int&>);
static_assert(!is_valid_type_v<int[]>);
static_assert(!is_valid_type_v<const int>);
static_assert(!is_valid_type_v<volatile int>);
template <typename A, typename B> struct MyPair { A a; B b; };
static_assert(is_valid_type_v<MyPair<int, int>>);
static_assert(!is_valid_type_v<MyPair<int, std::string>>);

// is_valid_obj
static_assert(is_valid_obj_v<std::string>);
static_assert(is_valid_obj_v<std::wstring>);
static_assert(is_valid_obj_v<std::vector<int>>);
static_assert(!is_valid_obj_v<std::vector<std::string>>);
#ifdef __cpp_lib_span
static_assert(is_valid_obj_v<std::span<char>>);
static_assert(is_valid_obj_v<std::span<int>>);
static_assert(!is_valid_obj_v<std::span<std::string>>);
#endif

// is_implicit_obj
static_assert(is_implicit_obj_v<std::string>);
static_assert(!is_implicit_obj_v<std::wstring>);
static_assert(!is_implicit_obj_v<std::vector<int>>);
static_assert(!is_implicit_obj_v<std::vector<std::string>>);
#ifdef __cpp_lib_span
static_assert(is_implicit_obj_v<std::span<char>>);
static_assert(!is_implicit_obj_v<std::span<int>>);
static_assert(!is_implicit_obj_v<std::span<std::string>>);
#endif

TEST(BytesViewTest, BasicTest) {
  int x[4];
  EXPECT_EQ(4 * sizeof(int), BytesView(x).size());

  std::string_view vs = BytesView(x);
  EXPECT_EQ(4 * sizeof(int), vs.size());
}

} // namespace cbu
