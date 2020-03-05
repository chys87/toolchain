/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

#include "memory.h"
#include <gtest/gtest.h>

namespace cbu {
inline namespace cbu_memory {

TEST(MemoryTest, MakeUnique) {
  EXPECT_EQ(sizeof(void*), sizeof(make_unique<int>(5)));
  EXPECT_EQ(2 * sizeof(void*), sizeof(make_unique<int[]>(5)));
  EXPECT_EQ(sizeof(void*), sizeof(make_unique_for_overwrite<int>()));
  EXPECT_EQ(2 * sizeof(void*), sizeof(make_unique_for_overwrite<int[]>(5)));

  EXPECT_EQ(5, *make_unique<int>(5));
  EXPECT_EQ(0, make_unique<int[]>(5)[0]);
  make_unique_for_overwrite<int>();
  make_unique_for_overwrite<int[]>(5);
}

class NonTrivialType {
 public:
  NonTrivialType() { ++constructors_; }
  NonTrivialType(const NonTrivialType&) { ++constructors_; }
  ~NonTrivialType() { ++destructors_; }

 public:
  static inline int constructors_ = 0;
  static inline int destructors_ = 0;
};

TEST(MemoryTest, MakeUniqueDestructorTest) {
  NonTrivialType::constructors_ = 0;
  NonTrivialType::destructors_ = 0;

  make_unique_for_overwrite<NonTrivialType[]>(5);

  EXPECT_EQ(5, NonTrivialType::constructors_);
  EXPECT_EQ(5, NonTrivialType::destructors_);
}

} // namespace cbu_memory
} // namespace cbu
