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

#include "cbu/common/compare.h"

#include <gtest/gtest.h>

namespace cbu {
namespace {

struct Comp {
  constexpr auto operator()(int a, int b) const noexcept {
    return a <=> b;
  }

  constexpr auto operator()(long a, long b) const noexcept {
    return a < b;
  }
};

static_assert(Weak_ordering_comparator<Comp, int, int>);
static_assert(!Weak_ordering_comparator<Comp, int, long>);
static_assert(!Weak_ordering_comparator<Comp, long, long>);
static_assert(!Lt_comparator<Comp, int, int>);
static_assert(!Lt_comparator<Comp, int, long>);
static_assert(Lt_comparator<Comp, long, long>);

static_assert(compare_weak_order_with(Comp(), 0, 1) ==
              std::strong_ordering::less);
static_assert(compare_weak_order_with(Comp(), 0L, 1L) ==
              std::strong_ordering::less);

static_assert(compare_lt_with(Comp(), 0, 1));
static_assert(compare_lt_with(Comp(), 0L, 1L));

} // namespace
} // namespace cbu
