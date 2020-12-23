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

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <new>

#include "cbu/compat/atomic_ref.h"
#include "cbu/common/defer.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
inline namespace cbu_init_guard {

// Generic implementation
class InitGuard {
 public:
  constexpr InitGuard() noexcept = default;
  InitGuard(const InitGuard&) = delete;
  InitGuard& operator=(const InitGuard&) = delete;

  template <typename Foo, typename... Args>
  bool init(Foo&& foo, Args&&... args)
      noexcept(noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    int v = std::atomic_ref(v_).load(std::memory_order_relaxed);
    if (v != DONE && guard_lock(v)) {
      bool normal = false;
      CBU_DEFER(
        if (normal)
          guard_release();
        else
          guard_abort();
      );
      std::forward<Foo>(foo)(std::forward<Args>(args)...);
      normal = true;
      return true;
    } else {
      return false;
    }
  }

  // IMPORTANT: The status may be changed after this function returns
  bool inited() const noexcept {
    return std::atomic_ref(v_).load(std::memory_order_relaxed) == DONE;
  }

  bool inited_no_race() const noexcept {
    return v_ == DONE;
  }

 private:
  bool guard_lock(int v) noexcept;
  void guard_release() noexcept;
  void guard_abort() noexcept;

 private:
  enum {
    INIT = 0,
    ABORTED = 1,
    RUNNING = 2,
    RUNNING_WAITING = 3,
    DONE = 4,
  };
  int v_ {INIT};
};

template <typename T>
class LazyInit {
 public:
  ~LazyInit() noexcept {
    if (guard_.inited_no_race()) {
      std::destroy_at(pointer());
    }
  }

  template <typename... Args>
  bool init(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))) {
    return guard_.init([&, this]() {
      // libc++ doesn't have std::construct_at yet
      new (pointer()) T(std::forward<Args>(args)...);
    });
  }

  // IMPORTANT: The status may be changed after this function returns
  bool inited() const noexcept {
    return guard_.inited();
  }

  T* pointer() noexcept {
    return std::launder(reinterpret_cast<T*>(buffer_));
  }
  const T* pointer() const noexcept {
    return std::launder(reinterpret_cast<const T*>(buffer_));
  }
  T& operator*() noexcept { return *pointer(); }
  const T& operator*() const noexcept { return *pointer(); }
  T* operator->() noexcept { return pointer(); }
  const T* operator->() const noexcept { return pointer(); }

 private:
  alignas(T) char buffer_[sizeof(T)];
  InitGuard guard_;
};

} // inline namespace cbu_init_guard
} // namespace cbu
