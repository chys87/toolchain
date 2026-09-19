/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "arena.h"

#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace cbu {
namespace {

struct DestructorLog {
  int id;
  std::vector<int>* log;

  ~DestructorLog() noexcept { log->push_back(id); }
};

struct ThrowOnSecondCopy {
  int id;
  std::vector<int>* log;

  ThrowOnSecondCopy(int id_, std::vector<int>* log_) : id(id_), log(log_) {}

  ThrowOnSecondCopy(const ThrowOnSecondCopy& other)
      : id(other.id), log(other.log) {
    if (id == 20) throw std::exception();
  }

  ~ThrowOnSecondCopy() noexcept { log->push_back(id); }
};

struct alignas(kArenaMaxAlign) MaxAlignedDestructor {
  ~MaxAlignedDestructor() noexcept {}
};

struct LargeOddSizedDestructor {
  char data[10001];

  ~LargeOddSizedDestructor() noexcept { ++destructors; }

  static inline int destructors = 0;
};

struct UnderAlignedDestructor {
  char data[3];

  ~UnderAlignedDestructor() noexcept { ++destructors; }

  static inline int destructors = 0;
};

}  // namespace

TEST(ArenaTest, BasicAllocation) {
  Arena arena;
  int* p = arena.emplace<int>(123);
  ASSERT_NE(nullptr, p);
  EXPECT_EQ(123, *p);

  char* returned = arena.Create<char[]>(4);
  arena.Return(returned, 4);
  char* reused = arena.Create<char[]>(2);
  EXPECT_EQ(returned, reused);

  int* bounded = arena.Create<int[3]>();
  static_assert(std::is_same_v<decltype(arena.Create<int[3]>()), int*>);
  bounded[0] = 4;
  EXPECT_EQ(4, bounded[0]);

  char* explicit_size_builder = arena.Create<char[]>(ByteSize<char>(2));
  explicit_size_builder[0] = 'x';
  EXPECT_EQ('x', explicit_size_builder[0]);

  int* explicit_size = arena.Create<int[]>(2);
  static_assert(std::is_same_v<decltype(arena.Create<int[]>(2)), int*>);
  explicit_size[0] = 5;
  explicit_size[1] = 6;
  EXPECT_EQ(5, explicit_size[0]);
  EXPECT_EQ(6, explicit_size[1]);

  char* unaligned_prefix = arena.Create<char[]>(3);
  (void)unaligned_prefix;
  char* aligned_tail = arena.Create<char[]>(ByteSize<char>(1), kArenaMaxAlign);
  EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(aligned_tail) % kArenaMaxAlign);
  arena.Return(aligned_tail, 1);
  char* after_return = arena.Create<char[]>(1);
  EXPECT_EQ(aligned_tail, after_return);

  char* explicit_aligned_tail = arena.Create<char[]>(1, kArenaMaxAlign);
  EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(explicit_aligned_tail) %
                   kArenaMaxAlign);

  std::span<int> array = arena.copy_n(std::vector{1, 2, 3});
  EXPECT_EQ(1, array[0]);
  EXPECT_EQ(2, array[1]);
  EXPECT_EQ(3, array[2]);

  std::string_view sv = arena.PushString("arena");
  EXPECT_EQ("arena", sv);

  zstring_view zs = arena.PushZString("zstring");
  EXPECT_STREQ("zstring", zs.c_str());

  EXPECT_EQ("builder", arena.PushString(sb::View{"builder"}));
  EXPECT_STREQ("zbuilder", arena.PushZString(sb::View{"zbuilder"}).c_str());
  EXPECT_EQ("xy", arena.PushString(sb::FromBuffer<3>{"xyz", 2}));
  EXPECT_STREQ("xy", arena.PushZString(sb::FromBuffer<3>{"xyz", 2}).c_str());
}

TEST(ArenaTest, MoveTransfersOwnership) {
  Arena first;
  int* p = first.emplace<int>(7);
  Arena second(std::move(first));
  EXPECT_EQ(7, *p);

  Arena third;
  int* moved = third.emplace<int>(8);
  second = std::move(third);
  EXPECT_EQ(8, *moved);
}

TEST(ArenaTest, ScalarDeleterDestructsInReverseOrder) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<DestructorLog> deleter;
    arena.emplace<DestructorLog>(deleter, 1, &log);
    arena.emplace<DestructorLog>(deleter, 2, &log);
    arena.emplace<DestructorLog>(deleter, 3, &log);
  }
  EXPECT_EQ((std::vector{3, 2, 1}), log);
}

TEST(ArenaTest, ArrayDeleterDestructsElementsBackward) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<DestructorLog[]> deleter;
    DestructorLog* p = arena.Create<DestructorLog[]>(3, deleter);
    for (int i = 0; i < 3; ++i) std::construct_at(p + i, i, &log);

    ArenaDeleter<DestructorLog[2]> bounded_deleter;
    static_assert(std::is_same_v<decltype(arena.Create<DestructorLog[2]>(
                                     bounded_deleter)),
                                 DestructorLog*>);
    DestructorLog* q = arena.Create<DestructorLog[2]>(bounded_deleter);
    for (int i = 0; i < 2; ++i) std::construct_at(q + i, 10 + i, &log);
  }
  EXPECT_EQ((std::vector{11, 10, 2, 1, 0}), log);
}

TEST(ArenaTest, ArrayDeleterSupportsMultidimensionalArrays) {
  std::vector<int> log;
  {
    Arena arena;
    using Row = DestructorLog[2];
    ArenaDeleter<Row[]> deleter;

    Row* p = arena.Create<DestructorLog[][2]>(2, deleter);
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) std::construct_at(p[i] + j, 2 * i + j, &log);
    }

    ArenaDeleter<DestructorLog[2][2]> bounded_deleter;
    Row* q = arena.Create<DestructorLog[2][2]>(bounded_deleter);
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j)
        std::construct_at(q[i] + j, 10 + 2 * i + j, &log);
    }
  }
  EXPECT_EQ((std::vector{12, 13, 10, 11, 2, 3, 0, 1}), log);
}

TEST(ArenaTest, TemplateArgumentsMustBeRaw) {
  static_assert(Arena_supported_type<int>);
  static_assert(!Arena_supported_type<void>);
  static_assert(!Arena_supported_type<int&>);
  static_assert(!Arena_supported_type<void()>);
  static_assert(!Arena_supported_type<const int>);
  static_assert(!Arena_supported_type<volatile int>);

  static_assert(Arena_supported_type<int[2]>);
  static_assert(!Arena_supported_type<const int[2]>);
  static_assert(Arena_supported_type<int[2][3]>);

  static_assert(Arena_supported_array<int[]>);
  static_assert(!Arena_supported_array<const int[]>);
  static_assert(!Arena_supported_array<volatile int[]>);
  static_assert(Arena_supported_array<int[][2]>);
  static_assert(!Arena_supported_array<const int[][2]>);
}

TEST(ArenaTest, DeleterDeducesElementType) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<DestructorLog> scalar_deleter;
    DestructorLog* scalar = arena.Create(scalar_deleter);
    std::construct_at(scalar, 1, &log);

    DestructorLog* emplaced = arena.emplace(scalar_deleter, 5, &log);
    EXPECT_EQ(5, emplaced->id);

    ArenaDeleter<DestructorLog[]> array_deleter;
    DestructorLog* from_count = arena.Create(1, array_deleter);
    std::construct_at(from_count, 2, &log);

    DestructorLog* from_integer = arena.Create(1, array_deleter);
    std::construct_at(from_integer, 3, &log);

    DestructorLog* aligned =
        arena.Create(1, alignof(DestructorLog), array_deleter);
    std::construct_at(aligned, 4, &log);
  }
  EXPECT_EQ((std::vector{4, 3, 2, 5, 1}), log);
}

TEST(ArenaTest, ArrayCopyRegistersDestructor) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<DestructorLog[]> deleter;
    std::vector<DestructorLog> source;
    source.reserve(2);
    source.emplace_back(1, &log);
    source.emplace_back(2, &log);
    auto copy = arena.copy_n(source.data(), source.size(), deleter);
    EXPECT_EQ(1, copy[0].id);
    EXPECT_EQ(2, copy[1].id);
    source.clear();
    log.clear();
  }
  EXPECT_EQ((std::vector{2, 1}), log);
}

TEST(ArenaTest, ArrayCopyDestroysConstructedElementsOnException) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<ThrowOnSecondCopy[]> deleter;
    std::vector<ThrowOnSecondCopy> source;
    source.reserve(2);
    source.emplace_back(10, &log);
    source.emplace_back(20, &log);
    EXPECT_THROW(arena.copy_n(source.data(), source.size(), deleter),
                 std::exception);
    source.clear();
  }
  EXPECT_EQ((std::vector{10, 10, 20}), log);
}

TEST(ArenaTest, DeleterNodesRespectAlignment) {
  std::vector<int> log;
  {
    Arena arena;
    ArenaDeleter<MaxAlignedDestructor> scalar_deleter;
    MaxAlignedDestructor* p =
        arena.Create<MaxAlignedDestructor>(scalar_deleter);
    EXPECT_EQ(
        0, reinterpret_cast<std::uintptr_t>(p) % alignof(MaxAlignedDestructor));

    ArenaDeleter<MaxAlignedDestructor[]> array_deleter;
    MaxAlignedDestructor* q = arena.Create<MaxAlignedDestructor[]>(
        3, alignof(MaxAlignedDestructor), array_deleter);
    EXPECT_EQ(
        0, reinterpret_cast<std::uintptr_t>(q) % alignof(MaxAlignedDestructor));
  }
}

TEST(ArenaTest, DeleterNodesMayRequirePaddingAndLargeNodes) {
  LargeOddSizedDestructor::destructors = 0;
  {
    Arena arena;
    ArenaDeleter<LargeOddSizedDestructor[]> deleter;
    (void)arena.Create<LargeOddSizedDestructor[]>(0, deleter);
    LargeOddSizedDestructor* p =
        arena.Create<LargeOddSizedDestructor[]>(2, deleter);
    std::construct_at(p);
    std::construct_at(p + 1);
  }
  EXPECT_EQ(2, LargeOddSizedDestructor::destructors);
}

TEST(ArenaTest, DeleterNodeFollowsUnderAlignedObject) {
  UnderAlignedDestructor::destructors = 0;
  static_assert(alignof(UnderAlignedDestructor) == 1);
  {
    Arena arena;
    ArenaDeleter<UnderAlignedDestructor> deleter;
    UnderAlignedDestructor* p = arena.Create<UnderAlignedDestructor>(deleter);
    std::construct_at(p);
  }
  EXPECT_EQ(1, UnderAlignedDestructor::destructors);
}

}  // namespace cbu
