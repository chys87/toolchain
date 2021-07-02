/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2021, chys <admin@CHYS.INFO>
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
#include <limits>
#include <memory>
#include <new>

#include "cbu/common/defer.h"
#include "cbu/compat/atomic_ref.h"

namespace cbu {

// Generic implementation
class InitGuard {
 public:
  constexpr InitGuard() noexcept = default;
  InitGuard(const InitGuard&) = delete;
  InitGuard& operator=(const InitGuard&) = delete;

  // Low-level interface.  Only use this if you know exactly what you're doing.
  enum struct LowLevelInit : int {};
  explicit constexpr InitGuard(LowLevelInit v) noexcept
      : v_(static_cast<int>(v)) {}

  // This is the low-level interface, the callback should specify the
  // value representing "DONE" to be stored in v_.
  // Only use this if you know exactly what you're doing.
  template <typename Foo, typename... Args>
  void init_with_reuse(Foo&& foo, Args&&... args) noexcept(
      noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    int v = std::atomic_ref(v_).load(std::memory_order_relaxed);
    if (!inited(v) && guard_lock(v)) {
      int done_value = ABORTED;
      CBU_DEFER({
        if (inited(done_value))
          guard_release(done_value);
        else
          guard_abort();
      });
      done_value = std::forward<Foo>(foo)(std::forward<Args>(args)...);
    }
  }

  // This is the normal interface you should use.
  template <typename Foo, typename... Args>
  void init(Foo&& foo, Args&&... args) noexcept(
      noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    init_with_reuse([&]() noexcept(noexcept(
                        std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
      std::forward<Foo>(foo)(std::forward<Args>(args)...);
      return DEFAULT_DONE;
    });
  }

  // IMPORTANT: The status may be changed after this function returns
  bool inited() const noexcept {
    return inited(std::atomic_ref(v_).load(std::memory_order_acquire));
  }

  bool inited_no_race() const noexcept { return inited(v_); }

  // After unit, the status is modified to ABORTED
  // (It's unsafe to reset it to INIT)
  bool uninit() noexcept;

  bool uninit_no_race() noexcept {
    if (inited(v_)) {
      v_ = ABORTED;
      return true;
    } else {
      return false;
    }
  }

  static constexpr bool inited(int v) noexcept { return v >= MIN_DONE; }

  // Use this only if you know exactly what you're doing
  // Typically it's only used by LazyScopedFD and similar situations
  constexpr const int* raw_value_ptr() const noexcept { return &v_; }
  constexpr int* raw_value_ptr(int v) noexcept { return &v_; }

 private:
  bool guard_lock(int v) noexcept;
  void guard_release(int v = DEFAULT_DONE) noexcept;
  void guard_abort() noexcept;

 public:
  // Intentionally use negative values, so that 0 and positive values can be
  // used for other purposes (e.g. as FD)
  enum {
    INIT = std::numeric_limits<int>::min(),
    ABORTED,
    RUNNING,
    RUNNING_WAITING,
    MIN_DONE,
    DEFAULT_DONE = 0,
    // All other values are also considered "DONE"
  };

 private:
  int v_{INIT};
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
  void init(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))) {
    guard_.init([&, this]() {
      // libc++ doesn't have std::construct_at yet
      new (pointer()) T(std::forward<Args>(args)...);
    });
  }

  // IMPORTANT: The status may be changed after this function returns
  bool inited() const noexcept { return guard_.inited(); }

  T* pointer() noexcept { return std::launder(reinterpret_cast<T*>(buffer_)); }
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

}  // namespace cbu
