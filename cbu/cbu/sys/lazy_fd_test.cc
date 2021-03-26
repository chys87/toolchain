/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021, chys <admin@CHYS.INFO>
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


#include "cbu/sys/lazy_fd.h"

#include <thread>
#include <vector>
#include<fcntl.h>

#include <gtest/gtest.h>

namespace cbu::inline cbu_init_guard {
namespace {

TEST(LazyFDTest, LazyFD) {
  {
    LazyFD lfd;
    EXPECT_LT(lfd.fd(), 0);
    lfd.init([]() noexcept { return open("/dev/null", O_RDONLY | O_CLOEXEC); });
    EXPECT_GE(lfd.fd(), 0);
  }

  {
    LazyFD lfd;
    EXPECT_LT(lfd.fd(), 0);

    std::vector<std::thread> threads;
    std::atomic<int> called = 0;
    for (int i = 0; i < 5; ++i) {
      threads.push_back(std::thread([&]() {
        usleep(10 * 1000);
        lfd.init([&]() {
          usleep(5 * 1000);
          ++called;
          return open("/dev/null", O_RDONLY | O_CLOEXEC);
        });
      }));
    }
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(called.load(), 1);
  }
}

}  // namespace
}  // namespace cbu::inline cbu_init_guard
