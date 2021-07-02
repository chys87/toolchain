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

#include "ref_cnt.h"

#include <atomic>

#include <gtest/gtest.h>

namespace cbu {
namespace {

struct MyType {
  MyType() { ctors++; }
  ~MyType() { dtors++; }

  static inline std::atomic<int> ctors;
  static inline std::atomic<int> dtors;
};

class RefCntPtrTest : public testing::Test {
 public:
  void SetUp() override {
    MyType::ctors = 0;
    MyType::dtors = 0;
  }
};

TEST_F(RefCntPtrTest, SingleThreadedTest) {
  {
    RefCntPtr<MyType> p1;
    RefCntPtr<MyType> p2{Construct{}};
    p1 = p2;

    EXPECT_EQ(p1.get(), p2.get());
    EXPECT_EQ(MyType::ctors, 1);
    EXPECT_EQ(MyType::dtors, 0);

    {
      RefCntPtr<MyType> p3 = std::move(p1);

      EXPECT_EQ(MyType::ctors, 1);
      EXPECT_EQ(MyType::dtors, 0);

      EXPECT_EQ(p1.get(), nullptr);
      EXPECT_EQ(p2.get(), p3.get());
    }

    p2 = nullptr;
    EXPECT_EQ(MyType::ctors, 1);
    EXPECT_EQ(MyType::dtors, 1);
  }

  EXPECT_EQ(MyType::ctors, 1);
  EXPECT_EQ(MyType::dtors, 1);
}

} // namespace
} // namespace cbu
