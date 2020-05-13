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

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <gtest/gtest.h>
#include <chrono>
#include "coroutine.h"

namespace cbu {
namespace coroutine {

TEST(CoRoutineTest, Basic) {

  std::vector<int> vec;

  CoContainer cont;
  cont.Register([&] {
    vec.push_back(1);
    Yield();
    vec.push_back(3);
  });

  cont.Register([&] {
    vec.push_back(2);
    Yield();
    vec.push_back(4);
  });

  cont.Run();

  EXPECT_EQ((std::vector<int>{1, 2, 3, 4}), vec);
}

TEST(CoRoutineTest, Sleep_Single) {
  CoContainer cont;

  cont.Register([&]{
    poll(nullptr, 0, 500);
  });

  auto start = std::chrono::steady_clock::now();
  cont.Run();
  auto end = std::chrono::steady_clock::now();
  auto seconds = std::chrono::duration<double>(end - start).count();
  EXPECT_LE(0.5, seconds);
  EXPECT_GT(0.7, seconds);
}

TEST(CoRoutineTest, Sleep_Multiple) {
  CoContainer cont;

  cont.Register([&]{
    poll(nullptr, 0, 300);
  });
  cont.Register([&]{
    poll(nullptr, 0, 500);
  });

  auto start = std::chrono::steady_clock::now();
  cont.Run();
  auto end = std::chrono::steady_clock::now();
  auto seconds = std::chrono::duration<double>(end - start).count();
  EXPECT_LE(0.5, seconds);
  EXPECT_GT(0.7, seconds);
}

TEST(CoRoutineTest, PipeTest) {
  int pipefds[2];
  ASSERT_EQ(0, pipe(pipefds));
  int res;
  int discard;

  CoContainer cont;

  cont.Register([&]{
    discard = read(pipefds[0], &res, sizeof(res));
    close(pipefds[0]);
  });
  cont.Register([&] {
    usleep(100000);
    int r = 2554;
    discard = write(pipefds[1], &r, sizeof(r));
    close(pipefds[1]);
  });

  auto start = std::chrono::steady_clock::now();
  cont.Run();
  auto end = std::chrono::steady_clock::now();
  auto seconds = std::chrono::duration<double>(end - start).count();
  EXPECT_LE(0.1, seconds);
  EXPECT_GT(0.2, seconds);
  EXPECT_EQ(2554, res);
}

TEST(CoRoutineTest, EpollTest) {
  int discard;
  int pipefds[2];
  int anotherpipefds[2];
  int res = 0;
  ASSERT_EQ(0, pipe2(pipefds, O_NONBLOCK | O_CLOEXEC));
  ASSERT_EQ(0, pipe2(anotherpipefds, O_CLOEXEC));

  int epfd = epoll_create1(EPOLL_CLOEXEC);
  {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLONESHOT;
    event.data.fd = pipefds[0];
    epoll_ctl(epfd, EPOLL_CTL_ADD, pipefds[0], &event);
  }
  {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLONESHOT;
    event.data.fd = anotherpipefds[0];
    epoll_ctl(epfd, EPOLL_CTL_ADD, anotherpipefds[0], &event);
  }

  CoContainer cont;
  cont.Register([&] {
    int rd = 0;
    for (int i = 0; i < 2; ++i) {
      struct epoll_event event;
      if (epoll_wait(epfd, &event, 1, -1) == 1) {
        discard = read(event.data.fd, &rd, sizeof(rd));
        res = res * 100 + rd;
      }
    }
    close(pipefds[0]);
    close(anotherpipefds[0]);
  });
  cont.Register([&] {
    usleep(100000);
    int r = 25;
    discard = write(pipefds[1], &r, sizeof(r));
    close(pipefds[1]);
  });
  cont.Register([&] {
    usleep(150000);
    int r = 54;
    discard = write(anotherpipefds[1], &r, sizeof(r));
    close(anotherpipefds[1]);
  });

  auto start = std::chrono::steady_clock::now();
  cont.Run();
  auto end = std::chrono::steady_clock::now();
  auto seconds = std::chrono::duration<double>(end - start).count();
  EXPECT_LE(0.15, seconds);
  EXPECT_GT(0.25, seconds);
  EXPECT_EQ(2554, res);
}

TEST(CoRoutineTest, WaitingTest) {
  int pipefds[2];
  ASSERT_EQ(0, pipe(pipefds));

  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  int discard;

  CoContainer cont;

  cont.Register([&]{
    auto writer_a = cont.Register([&]{
      uint32_t x = 25;
      usleep(100000);
      discard = write(pipefds[1], &x, sizeof(x));
      usleep(200000);
      x = 54;
      discard = write(pipefds[1], &x, sizeof(x));
    });
    auto writer_b = cont.Register([&]{
      WaitFor(writer_a);
      uint32_t x = 79;
      discard = write(pipefds[1], &x, sizeof(x));
    });
    cont.Register([&]{
      WaitFor(writer_b);
      uint32_t x = 133;
      discard = write(pipefds[1], &x, sizeof(x));
    });

    auto id_a = cont.Register([&]{
      discard = read(pipefds[0], &a, sizeof(a));
    });
    auto id_b = cont.Register([&]{
      WaitFor(id_a);
      discard = read(pipefds[0], &b, sizeof(b));
    });
    auto id_c = cont.Register([&]{
      WaitFor(id_a);
      WaitFor(id_b);
      discard = read(pipefds[0], &c, sizeof(c));
    });
    WaitFor(id_c);
    WaitFor(id_b);
    discard = read(pipefds[0], &d, sizeof(d));
  });
  cont.Run();

  close(pipefds[0]);
  close(pipefds[1]);

  EXPECT_EQ(25, a);
  EXPECT_EQ(54, b);
  EXPECT_EQ(79, c);
  EXPECT_EQ(133, d);
}

} // namespace coroutine
} // namespace cbu
