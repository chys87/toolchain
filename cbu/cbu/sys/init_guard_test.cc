/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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


#include "cbu/sys/init_guard.h"

#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace cbu {
inline namespace cbu_init_guard {
namespace {

constexpr int sleep_unit = 100 * 1000;

TEST(InitGuardTest, InitGuard) {
  InitGuard guard;
  std::atomic<int> succ{0};
  std::atomic<int> throws{0};
  std::atomic<int> caught{0};

  auto throwing = [&](int us) {
    usleep(us);
    try {
      guard.init([&](int throw_val) {
        usleep(3 * sleep_unit);
        ++throws;
        throw throw_val;
      }, 5);
    } catch (int) {
      ++caught;
    }
  };
  auto nonthrowing = [&](int us) {
    usleep(us);
    guard.init([&]() {
      usleep(3 * sleep_unit);
      ++succ;
    });
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.push_back(std::thread(throwing, i * sleep_unit));
  }
  for (int i = 0; i < 5; ++i) {
    threads.push_back(std::thread(nonthrowing, (i + 5) * sleep_unit));
  }

  for (auto& thr: threads) {
    thr.join();
  }

  EXPECT_EQ(succ.load(), 1);
  EXPECT_LE(throws.load(), 6);
  EXPECT_GE(throws.load(), 4);
}

struct Cls {
  Cls() { ++ctor; }
  ~Cls() { ++dtor; }

  static inline std::atomic<int> ctor{0};
  static inline std::atomic<int> dtor{0};
};

TEST(InitGuardTest, LazyInit) {
  {
    LazyInit<Cls> lz;
    EXPECT_FALSE(lz.inited());
    EXPECT_TRUE(lz.init());
    EXPECT_FALSE(lz.init());
    EXPECT_TRUE(lz.inited());
  }
  EXPECT_EQ(Cls::ctor.load(), 1);
  EXPECT_EQ(Cls::dtor.load(), 1);
}

} // namespace
} // inline namespace cbu_init_guard
} // namespace cbu
