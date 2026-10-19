/*
 * cbu - chys's basic utilities
 * Copyright (c) 2026, chys <admin@CHYS.INFO>
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

#include "cbu/common/no_destroy.h"

#include <gtest/gtest.h>

namespace {

struct Tracked {
  static int destructions;

  constexpr explicit Tracked(int value) : value(value) {}

  ~Tracked() { ++destructions; }

  int value;
};

int Tracked::destructions = 0;

struct Pair {
  int first;
  int second;
};

struct Default {
  constexpr Default() : value(11) {}

  ~Default() { ++Tracked::destructions; }

  int value;
};

CBU_DEFINE_NO_DESTROY(Tracked, no_destroy, 42);
CBU_DEFINE_NO_DESTROY(Pair, no_destroy_pair, 1, 2);
CBU_DEFINE_NO_DESTROY(Default, no_destroy_default);
CBU_CONSTINIT_NO_DESTROY(Tracked, constinit_no_destroy, 7);
CBU_CONSTINIT_NO_DESTROY(Pair, constinit_no_destroy_pair, 3, 4);
CBU_CONSTINIT_NO_DESTROY(Default, constinit_no_destroy_default);

}  // namespace

TEST(NoDestroyTest, MacroDefinesUsableObjects) {
  EXPECT_EQ(42, no_destroy.value);
  EXPECT_EQ(1, no_destroy_pair.first);
  EXPECT_EQ(2, no_destroy_pair.second);
  EXPECT_EQ(11, no_destroy_default.value);

  EXPECT_EQ(7, constinit_no_destroy.value);
  EXPECT_EQ(3, constinit_no_destroy_pair.first);
  EXPECT_EQ(4, constinit_no_destroy_pair.second);
  EXPECT_EQ(11, constinit_no_destroy_default.value);
}

TEST(NoDestroyTest, DestructorDoesNotDestroyValue) {
  Tracked::destructions = 0;

  {
    cbu::NoDestroy<Tracked> value{123};
    EXPECT_EQ(123, value.value.value);
  }

  EXPECT_EQ(0, Tracked::destructions);
}
