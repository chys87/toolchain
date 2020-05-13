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

#include "coroutine.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <exception>
#include <map>
#include "cbu/common/byte_size.h"
#include "cbu/coroutine/syscall_hook.h"

namespace cbu {
namespace coroutine {

__thread CoContainer* active_container = nullptr;

void Stack::Allocate(size_t sentinel_size, size_t stack_size) {
  size_t total_size = sentinel_size + stack_size;

  // Don't use MAP_GROWSDOWN - that actually allows almost unlimited stack size
  sentinel_ = mmap(
      nullptr, total_size,
      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
      -1, 0);
  if (sentinel_ == nullptr) {
    throw std::bad_alloc();
  }
  if (sentinel_size) {
    mprotect(sentinel_, sentinel_size, PROT_NONE);
  }
  lo_ = byte_advance(sentinel_, sentinel_size);
  hi_ = byte_advance(sentinel_, total_size);
}

void Stack::Deallocate() noexcept {
  if (sentinel_ != nullptr) {
    munmap(sentinel_, byte_distance(sentinel_, hi_));
    sentinel_ = nullptr;
    lo_ = nullptr;
    hi_ = nullptr;
  }
}

CoContainer::CoContainer(Attr attr) : attr_(attr) {
  // Push scheduler as the 0-th coroutine
  std::unique_ptr<CoRoutine> scheduler(new CoRoutine);
  scheduler->status = Status::RUNNING;
  co_list_.push_back(std::move(scheduler));
}

CoId CoContainer::Register(CoFunc func) {
  size_t id = co_list_.size();
  co_list_.push_back(MakeCoRoutine(id, std::move(func)));
  ready_list_.push(id);
  return id;
}

void CoContainer::Run() {
  if (co_list_.size() <= 1)
    return;

  active_container = this;

  for (;;) {
    size_t run_idx = 0;

    if (!ready_list_.empty()) {
      run_idx = ready_list_.front();
      ready_list_.pop();
    } else if (!io_wait_list_.empty()) {
      DoPoll();
      // When DoPoll returns, ready_list_ should not be empty
      run_idx = ready_list_.front();
      ready_list_.pop();
    }

    if (run_idx == 0)
      break;

    CoRoutine* coroutine = co_list_[run_idx].get();
    coroutine->status = Status::RUNNING;
    current_id_ = run_idx;
    SwitchContext(co_list_[0].get(), coroutine);

    // Clean up finished coroutines
    if (coroutine->status == Status::DONE) {
      for (CoId id: coroutine->waited_by) {
        co_list_[id]->status = Status::READY;
        ready_list_.push(id);
      }
      co_list_[run_idx].reset();
    }
  }

  active_container = nullptr;

  co_list_.resize(1);
}

// Poll all io-waiting coroutines, and move io-ready ones to ready list
void CoContainer::DoPoll() {
  for (;;) {
    std::vector<pollfd> poll_fds;

    std::chrono::steady_clock::time_point expire_time = \
        IoWaitInfo::kNoExpireTime;
    for (CoId id: io_wait_list_) {
      auto* coroutine = co_list_[id].get();
      auto& io_wait_info = coroutine->io_wait_info;
      expire_time = std::min(expire_time, io_wait_info.expire_time);
      poll_fds.insert(poll_fds.end(),
                      io_wait_info.fds,
                      io_wait_info.fds + io_wait_info.nfds);
    }

    int timeout_ms = -1;
    if (expire_time < IoWaitInfo::kNoExpireTime) {
      auto now = std::chrono::steady_clock::now();
      if (now >= expire_time) {
        timeout_ms = 0;
      } else {
        // Plus 999999 nanoseconds so that we round up
        timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            expire_time - now + std::chrono::nanoseconds(999999)).count();
      }
    }
    int ret = sys_poll(poll_fds.data(), poll_fds.size(), timeout_ms);
    if (ret < 0) {
      // This is not likely, but we need to handle them.
      for (CoId id: io_wait_list_) {
        auto* coroutine = co_list_[id].get();
        coroutine->status = Status::READY;
        coroutine->io_wait_info.ret = ret;
        ready_list_.push(id);
      }
      io_wait_list_.clear();
      return;
    }

    // Collect returned fd status
    std::map<int, uint16_t> revents_map;
    for (auto& item: poll_fds) {
      if (item.revents != 0) {
        revents_map[item.fd] |= item.revents;
      }
    }

    // Check which coroutines are now ready
    for (auto it = io_wait_list_.begin(); it != io_wait_list_.end(); ) {
      CoId id = *it;
      auto* coroutine = co_list_[id].get();
      auto& io_wait_info = coroutine->io_wait_info;
      int ready_count = 0;
      for (size_t k = 0, m = io_wait_info.nfds; k < m; ++k) {
        auto& item = io_wait_info.fds[k];
        auto it_fd = revents_map.find(item.fd);
        if (it_fd != revents_map.end()) {
          item.revents = it_fd->second & (
              item.events | POLLERR | POLLHUP | POLLNVAL);
          if (item.revents != 0)
            ++ready_count;
        } else {
          item.revents = 0;
        }
      }

      bool done = false;
      if (ready_count != 0) {
        io_wait_info.ret = ready_count;
        done = true;
      } else if (io_wait_info.expire_time < io_wait_info.kNoExpireTime &&
                 std::chrono::steady_clock::now() >= io_wait_info.expire_time) {
        io_wait_info.ret = 0;
        done = true;
      }

      if (done) {
        coroutine->status = Status::READY;
        ready_list_.push(id);
        it = io_wait_list_.erase(it);
      } else {
        ++it;
      }
    }
    if (!ready_list_.empty())
      return;
  }
}

void CoContainer::Yield() {
  SwitchToScheduler(Status::READY);
}

int CoContainer::Poll(pollfd* fds, nfds_t nfds, int timeout_ms) {
  if (nfds == 0) {
    // Used as sleeping
    if (timeout_ms <= 0) {
      // The kernel doesn't reject nfds == 0 && timeout_ms < 0, in which case
      // poll works pretty much like sigpending.
      // But we don't support such usage.
      return 0;
    }
  } else {
    // Do a non-waiting poll first, in case some arguments are invalid, and
    // in case some fds are already ready.
    int ret = sys_poll(fds, nfds, 0);
    if (ret != 0 || timeout_ms == 0)
      return ret;
  }

  // Push coroutine to io-waiting list
  auto* coroutine = co_list_[current_id_].get();
  coroutine->io_wait_info.fds = fds;
  coroutine->io_wait_info.nfds = nfds;

  if (timeout_ms < 0) {
    coroutine->io_wait_info.expire_time = IoWaitInfo::kNoExpireTime;
  } else {
    coroutine->io_wait_info.expire_time = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(timeout_ms);
  }

  SwitchToScheduler(Status::WAITING_IO);
  return coroutine->io_wait_info.ret;
}

bool CoContainer::WaitFor(CoId other_id) {
  if (current_id_ == 0)
    return false;
  if (other_id == 0 || other_id >= co_list_.size() || other_id == current_id_)
    return false;
  // Is the other coroutine already finished?
  CoRoutine* coroutine = co_list_[other_id].get();
  if (coroutine == nullptr || coroutine->status == Status::DONE)
    return true;
  // Any circles?
  CoId id = other_id;
  while (co_list_[id]->status == Status::WAITING_OTHER) {
    id = co_list_[id]->waiting_for;
    if (id == current_id_)
      return false;
  }
  // Let's do it
  co_list_[current_id_]->waiting_for = other_id;
  coroutine->waited_by.push_back(current_id_);
  SwitchToScheduler(Status::WAITING_OTHER);
  return true;
}

void CoContainer::SwitchToScheduler(Status new_status) {
  size_t current_id = std::exchange(current_id_, 0);
  co_list_[current_id]->status = new_status;
  switch (new_status) {
    case Status::READY:
      ready_list_.push(current_id);
      break;
    case Status::WAITING_IO:
      io_wait_list_.insert(current_id);
      break;
    default:
      break;
  }
  SwitchContext(co_list_[current_id].get(), co_list_[0].get());
}

void CoContainer::CoRoutineWrapper(CoFunc& func) {
  try {
    if (func)
      func();
  } catch (const std::exception& e) {
    fprintf(stderr, "Coroutine throws exception %s: %s\n",
            typeid(e).name(), e.what());
    std::terminate();
  } catch (...) {
    fprintf(stderr, "Coroutine throws unknown exception\n");
    std::terminate();
  }
  active_container->SwitchToScheduler(Status::DONE);
  __builtin_trap();
}

std::unique_ptr<CoRoutine> CoContainer::MakeCoRoutine(
    CoId id, CoFunc func) {
  std::unique_ptr<CoRoutine> coroutine(new CoRoutine);
  coroutine->id = id;
  coroutine->func = std::move(func);
  coroutine->stack.Allocate(attr_.stack_sentinel_size, attr_.stack_size);

  // x86-64 ABI expects stack to be aligned to 16 bytes *before* calling
  // a function, so we subtract by 8.
  coroutine->context.rsp =
    reinterpret_cast<uint64_t>(coroutine->stack.hi()) - 8;
  coroutine->context.rip = reinterpret_cast<uint64_t>(&CoRoutineWrapper);
  coroutine->context.startup_context =
    reinterpret_cast<uint64_t>(&coroutine->func);
  return coroutine;
}

} // namespace coroutine
} // namespace cbu
