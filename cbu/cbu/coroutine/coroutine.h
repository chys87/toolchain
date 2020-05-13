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

#pragma once

#include <poll.h>
#include <stdint.h>
#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <vector>

namespace cbu {
namespace coroutine {

struct Context {
  uint64_t rbx;
  uint64_t rbp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t rip;
  uint64_t rsp;
  uint64_t startup_context;  // Restored to RDI (for CoRoutineWrapper)
};

enum struct Status: unsigned char {
  READY,  // Ready to continue running
  RUNNING,  // Currently running
  WAITING_IO,  // Waiting for IO
  WAITING_OTHER,  // Waiting for another coroutine to finish
  DONE,  // Exited
};

// 0 is scheduler; 1, 2, are real coroutines
// CoId is also the index in co_list_
using CoId = uint32_t;

using CoFunc = std::function<void()>;

class Stack {
 public:
  Stack() = default;
  Stack(const Stack&) = delete;
  Stack& operator=(const Stack&) = delete;
  ~Stack() { Deallocate(); }

  void Allocate(size_t sentinel_size, size_t stack_size);
  void Deallocate() noexcept;

  void* sentinel() const noexcept { return sentinel_; }
  void* lo() const noexcept { return lo_; }
  void* hi() const noexcept { return hi_; }

 private:
  void* sentinel_ = nullptr;
  void* lo_ = nullptr;
  void* hi_ = nullptr;
};

struct IoWaitInfo {
  static constexpr auto kNoExpireTime = \
      std::chrono::steady_clock::time_point::max();
  std::chrono::steady_clock::time_point expire_time = kNoExpireTime;

  pollfd* fds = nullptr;
  nfds_t nfds = 0;
  int ret = -1;  // Return value of poll
};

struct CoRoutine {
  // context must be the first field (assembler code uses this)
  Context context = {};
  CoFunc func;
  CoId id = 0;
  Stack stack;
  Status status = Status::READY;
  IoWaitInfo io_wait_info;  // Only useful if status == Status::WAITING_IO
  CoId waiting_for = 0;  // Only useful if status == Status::WAITING_OTHER
  std::vector<CoId> waited_by;  // Who's waiting for me?
};

struct Attr {
  size_t stack_size = 64 * 1024;
  size_t stack_sentinel_size = 8192;
};

class CoContainer {
 public:
  explicit CoContainer(Attr attr = {});

  CoId Register(CoFunc func);

  void Run();

  CoId Self() const { return current_id_; }

  // The following cannot be called from the main coroutine
  void Yield();
  int Poll(pollfd* fds, nfds_t nfds, int timeout_ms = -1);
  bool WaitFor(CoId other_id);

 private:
  void DoPoll();
  std::unique_ptr<CoRoutine> MakeCoRoutine(CoId id, CoFunc func);
  void SwitchToScheduler(Status new_status);

  [[noreturn]] static void CoRoutineWrapper(CoFunc& func);

 private:
  Attr attr_;
  CoId current_id_ = 0;
  std::vector<std::unique_ptr<CoRoutine>> co_list_;
  std::queue<CoId> ready_list_;
  std::set<CoId> io_wait_list_;
};

// thread_local generates longer code in non-LTO builds
extern __thread CoContainer* active_container;

void SwitchContext(CoRoutine* from, const CoRoutine* to) noexcept
  asm("cbu_coroutine_switch_context");

inline void Yield() {
  active_container->Yield();
}

inline CoId Self() {
  return active_container->Self();
}

inline bool WaitFor(CoId other_id) {
  return active_container->WaitFor(other_id);
}

} // namespace coroutine
} // namespace cbu
