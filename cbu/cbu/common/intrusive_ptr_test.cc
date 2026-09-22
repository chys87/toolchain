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
 * (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "intrusive_ptr.h"

#include <gtest/gtest.h>

namespace cbu {
namespace {

class IntrusiveVirtual {
 public:
  constexpr IntrusiveVirtual() noexcept = default;
  virtual ~IntrusiveVirtual() noexcept = 0;
  CBU_DECLARE_INTRUSIVE_PTR_CLASS(IntrusiveVirtual);
};

inline IntrusiveVirtual::~IntrusiveVirtual() noexcept = default;

class TestType : public IntrusiveVirtual {
 public:
  TestType() { ++ctors; }
  TestType(const TestType&) = delete;
  TestType& operator=(const TestType&) = delete;
  ~TestType() { ++dtors; }

  static inline int ctors = 0;
  static inline int dtors = 0;
};

class DerivedTestType : public TestType {
 public:
};

class IntrusivePtrTest : public ::testing::Test {
 public:
  void SetUp() override {
    TestType::ctors = 0;
    TestType::dtors = 0;
  }
};

TEST_F(IntrusivePtrTest, Basic) {
  {
    intrusive_ptr<TestType> p(new TestType);
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);
    EXPECT_TRUE(p);
    EXPECT_NE(p.get(), nullptr);
    EXPECT_EQ(p.operator->(), p.get());
  }

  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 1);
}

TEST_F(IntrusivePtrTest, CopyAndMove) {
  {
    intrusive_ptr<TestType> p(new TestType);
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr<TestType> q(p);
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr<TestType> r(std::move(p));
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    EXPECT_EQ(p.get(), nullptr);
    EXPECT_NE(q.get(), nullptr);
    EXPECT_NE(r.get(), nullptr);
    EXPECT_EQ(q.get(), r.get());
  }

  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 1);
}

TEST_F(IntrusivePtrTest, InPlaceConstruct) {
  {
    intrusive_ptr<IntrusiveVirtual> p{std::in_place_type<TestType>};
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr q(p);
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr r(std::move(p));
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);
  }

  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 1);
}

TEST_F(IntrusivePtrTest, DerivedType) {
  {
    intrusive_ptr<DerivedTestType> dtt{std::in_place};
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr<IntrusiveVirtual> iv(dtt);

    iv = intrusive_ptr<TestType>(std::move(dtt));

    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 0);
  }

  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 1);
}

TEST_F(IntrusivePtrTest, ResetAndAssign) {
  {
    intrusive_ptr<TestType> p(new TestType);
    p.reset();
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_EQ(TestType::ctors, 1);
    EXPECT_EQ(TestType::dtors, 1);

    p.reset();  // Resetting an empty pointer is a no-op
    EXPECT_EQ(TestType::dtors, 1);

    intrusive_ptr<TestType> q(new TestType);
    p = q;
    EXPECT_EQ(p.get(), q.get());
    EXPECT_NE(p.get(), nullptr);

    p = nullptr;
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_NE(q.get(), nullptr);
    EXPECT_EQ(TestType::dtors, 1);
  }

  EXPECT_EQ(TestType::ctors, 2);
  EXPECT_EQ(TestType::dtors, 2);
}

TEST_F(IntrusivePtrTest, Swap) {
  intrusive_ptr<TestType> p(new TestType);
  intrusive_ptr<TestType> q;
  p.swap(q);
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_NE(q.get(), nullptr);
  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 0);
}

TEST_F(IntrusivePtrTest, ReleaseAndAdopt) {
  {
    intrusive_ptr<TestType> p(new TestType);
    TestType* raw = p.release_unsafe();
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_EQ(TestType::dtors, 0);

    intrusive_ptr<TestType> q(kUnsafe, raw);
    EXPECT_EQ(q.get(), raw);
    EXPECT_EQ(TestType::dtors, 0);
  }

  EXPECT_EQ(TestType::ctors, 1);
  EXPECT_EQ(TestType::dtors, 1);
}

TEST_F(IntrusivePtrTest, Renew) {
  {
    intrusive_ptr<TestType> p(new TestType);
    p.renew();
    EXPECT_NE(p.get(), nullptr);
    EXPECT_EQ(TestType::ctors, 2);
    EXPECT_EQ(TestType::dtors, 1);
  }

  EXPECT_EQ(TestType::ctors, 2);
  EXPECT_EQ(TestType::dtors, 2);
}

}  // namespace
}  // namespace cbu
