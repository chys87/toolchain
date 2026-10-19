/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
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

#include <new>

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

struct ThrowingCtorException {};

// Over-aligned so that OutlinableArray's heap path goes through the aligned
// array new/delete operators, which we count below.
struct alignas(256) ThrowingCtorType {
  ThrowingCtorType() {
    if (constructors_ == throw_at_) throw ThrowingCtorException{};
    ++constructors_;
  }
  ~ThrowingCtorType() { ++destructors_; }

  static inline int constructors_ = 0;
  static inline int destructors_ = 0;
  static inline int throw_at_ = -1;
};

int g_aligned_array_news = 0;
int g_aligned_array_deletes = 0;

}  // namespace cbu

void* operator new[](std::size_t n, std::align_val_t a) {
  ++cbu::g_aligned_array_news;
  return ::operator new(n, a);
}

void operator delete[](void* p, std::align_val_t a) noexcept {
  ++cbu::g_aligned_array_deletes;
  ::operator delete(p, a);
}

void operator delete[](void* p, std::size_t n, std::align_val_t a) noexcept {
  ++cbu::g_aligned_array_deletes;
  ::operator delete(p, n, a);
}

namespace cbu {

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
  sized_unique_ptr<int[]>(nullptr, ArrayDeleter<int>{});
  sized_unique_ptr<int[]>(nullptr, ArrayDeleter<int>{1});
  sized_unique_ptr<NonTrivialType[]>();
  sized_unique_ptr<NonTrivialType[]>(nullptr);
  sized_unique_ptr<NonTrivialType[]>(nullptr, ArrayDeleter<NonTrivialType>{});
  sized_unique_ptr<NonTrivialType[]>(nullptr, ArrayDeleter<NonTrivialType>{1});
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

TEST(MemoryTest, OutlinableArrayForOverwrite) {
  OutlinableArrayBuffer<int, 5> buf;
  OutlinableArray<int> arr(&buf, 3,
                           OutlinableArray<int>::for_overwrite);
  ASSERT_EQ(3, arr.size());
  ASSERT_EQ(arr.get(), static_cast<void*>(buf.buffer));
  for (int k = 0; k < 3; ++k) arr[k] = k;
  EXPECT_EQ(2, arr[2]);
}

TEST(MemoryTest, OutlinableArrayNonTrivial) {
  NonTrivialType::constructors_ = 0;
  NonTrivialType::destructors_ = 0;

  {
    OutlinableArrayBuffer<NonTrivialType, 5> buf;
    OutlinableArray<NonTrivialType> arr(
        &buf, 3, OutlinableArray<NonTrivialType>::for_overwrite);
    ASSERT_EQ(3, arr.size());
    EXPECT_EQ(3, NonTrivialType::constructors_);
  }
  EXPECT_EQ(3, NonTrivialType::destructors_);

  {
    // Heap allocation path
    OutlinableArrayBuffer<NonTrivialType, 2> buf;
    OutlinableArray<NonTrivialType> arr(
        &buf, 5, OutlinableArray<NonTrivialType>::for_overwrite);
    ASSERT_EQ(5, arr.size());
    EXPECT_EQ(8, NonTrivialType::constructors_);
  }
  EXPECT_EQ(8, NonTrivialType::destructors_);
}

TEST(MemoryTest, OutlinableArrayConstructorThrows) {
  ThrowingCtorType::constructors_ = 0;
  ThrowingCtorType::destructors_ = 0;
  ThrowingCtorType::throw_at_ = 2;

  // Heap path: the raw memory must be freed if a constructor throws
  OutlinableArrayBuffer<ThrowingCtorType, 2> buf;
  int old_news = g_aligned_array_news;
  int old_deletes = g_aligned_array_deletes;
  bool caught = false;
  try {
    OutlinableArray<ThrowingCtorType> arr(&buf, 5);
  } catch (const ThrowingCtorException&) {
    caught = true;
  }
  EXPECT_TRUE(caught);
  EXPECT_EQ(2, ThrowingCtorType::constructors_);
  EXPECT_EQ(2, ThrowingCtorType::destructors_);
  EXPECT_EQ(old_news + 1, g_aligned_array_news);
  EXPECT_EQ(old_deletes + 1, g_aligned_array_deletes);

  // Stack path: no allocation, constructed elements are destroyed
  OutlinableArrayBuffer<ThrowingCtorType, 5> buf2;
  ThrowingCtorType::constructors_ = 0;
  ThrowingCtorType::destructors_ = 0;
  ThrowingCtorType::throw_at_ = 1;
  old_news = g_aligned_array_news;
  old_deletes = g_aligned_array_deletes;
  caught = false;
  try {
    OutlinableArray<ThrowingCtorType> arr(
        &buf2, 3, OutlinableArray<ThrowingCtorType>::for_overwrite);
  } catch (const ThrowingCtorException&) {
    caught = true;
  }
  EXPECT_TRUE(caught);
  EXPECT_EQ(1, ThrowingCtorType::constructors_);
  EXPECT_EQ(1, ThrowingCtorType::destructors_);
  EXPECT_EQ(old_news, g_aligned_array_news);
  EXPECT_EQ(old_deletes, g_aligned_array_deletes);
}

}  // namespace cbu
