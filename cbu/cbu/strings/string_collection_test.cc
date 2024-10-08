/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2024, chys <admin@CHYS.INFO>
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

#include "cbu/strings/string_collection.h"

#include <type_traits>

namespace cbu {
namespace {

using std::operator""sv;

using S = string_collection<"a", "ab", "abc", "bc", "xyz">;

static_assert(S::ref<"a"> == "a"sv);
static_assert(S::ref<"ab"> == "ab"sv);
static_assert(S::ref<"abc"> == "abc"sv);
static_assert(S::ref<"bc"> == "bc"sv);
static_assert(S::ref<"xyz"> == "xyz"sv);

static_assert(S::ref<"a">.data() == S::ref<"abc">.data());
static_assert(S::ref<"ab">.data() == S::ref<"abc">.data());
static_assert(S::ref<"bc">.data() == S::ref<"abc">.data() + 1);

// Equal length (doesn't actually store length)
using S1 = string_collection<"hello", "world", "fucku">;
static_assert(sizeof(S1::ref_t) == 1);
static_assert(std::is_same_v<S1::ref_t, S1::ref_regular_t>);

// Packed storage
static_assert(sizeof(S::ref_t) == 1);
static_assert(std::is_same_v<S::ref_t, S::ref_packed_t>);

}  // namespace
}  // namespace cbu
