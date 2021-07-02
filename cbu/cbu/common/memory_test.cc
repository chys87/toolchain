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

struct alignas(256) OverAligned {
  int x;
};

TEST(MemoryTest, MakeUnique) {
  EXPECT_EQ(sizeof(void*), sizeof(make_unique<int>(5)));
  EXPECT_EQ(2 * sizeof(void*), sizeof(make_unique<int[]>(5)));
  EXPECT_EQ(sizeof(void*), sizeof(make_unique_for_overwrite<int>()));
  EXPECT_EQ(2 * sizeof(void*), sizeof(make_unique_for_overwrite<int[]>(5)));

  EXPECT_EQ(5, *make_unique<int>(5));
  EXPECT_EQ(0, make_unique<int[]>(5)[0]);
  make_unique_for_overwrite<int>();
  make_unique_for_overwrite<int[]>(5);

  {
    auto ptr = make_unique<OverAligned>();
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
  {
    auto ptr = make_unique<OverAligned[]>(5);
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
  {
    auto ptr = make_unique_for_overwrite<OverAligned[]>(5);
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
}

TEST(MemoryTest, MakeShared) {
  EXPECT_EQ(5, *make_shared<int>(5));
  EXPECT_EQ(0, make_shared<int[]>(5)[0]);
  make_shared_for_overwrite<int>();
  make_shared_for_overwrite<int[]>(5);

  {
    std::shared_ptr<int[]> p = make_shared<int[]>(5);
    std::shared_ptr<int> q(p, p.get());
    EXPECT_EQ(q.get(), p.get());
    p[0] = 42;
    EXPECT_EQ(*q, 42);
  }

  {
    auto ptr = make_shared<OverAligned>();
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
  {
    auto ptr = make_shared<OverAligned[]>(5);
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
  {
    auto ptr = make_shared_for_overwrite<OverAligned[]>(5);
    EXPECT_EQ(uintptr_t(ptr.get()) % alignof(OverAligned), 0);
  }
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

TEST(MemoryTest, MakeSharedDestructorTest) {
  NonTrivialType::constructors_ = 0;
  NonTrivialType::destructors_ = 0;

  {
    auto p1 = make_shared_for_overwrite<NonTrivialType[]>(5);
    std::shared_ptr<NonTrivialType> p2(p1, p1.get());
    std::shared_ptr<NonTrivialType> p3(p1, p1.get() + 3);
  }

  EXPECT_EQ(5, NonTrivialType::constructors_);
  EXPECT_EQ(5, NonTrivialType::destructors_);
}

TEST(MemoryTest, MakeUniqueNullPointer) {
  sized_unique_ptr<int[]>();
  sized_unique_ptr<int[]>(nullptr);
  sized_unique_ptr<int[]>(nullptr, SizedArrayDeleter<int>{});
  sized_unique_ptr<int[]>(nullptr, SizedArrayDeleter<int>{sizeof(int)});
  sized_unique_ptr<NonTrivialType[]>();
  sized_unique_ptr<NonTrivialType[]>(nullptr);
  sized_unique_ptr<NonTrivialType[]>(nullptr,
                                     SizedArrayDeleter<NonTrivialType>{});
  sized_unique_ptr<NonTrivialType[]>(
      nullptr, SizedArrayDeleter<NonTrivialType>{sizeof(NonTrivialType)});
}

TEST(MemoryTest, OutlinableArray) {
  {
    OutlinableArrayBuffer<int, 5> buf;

    {
      OutlinableArray<int> arr(&buf, 3);
      ASSERT_EQ(arr.get(), static_cast<void*>(buf.buffer));
    }
    {
      OutlinableArray<int> arr(&buf, 6);
      ASSERT_NE(arr.get(), static_cast<void*>(buf.buffer));
    }
  }
  {
    OutlinableArrayBuffer<OverAligned, 5> buf;

    {
      OutlinableArray<OverAligned> arr(&buf, 5);
      ASSERT_EQ(arr.get(), static_cast<void*>(buf.buffer));
    }
    {
      OutlinableArray<OverAligned> arr(&buf, 6);
      ASSERT_NE(arr.get(), static_cast<void*>(buf.buffer));
    }
  }
}

}  // namespace cbu
