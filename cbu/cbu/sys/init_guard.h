/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2023, chys <admin@CHYS.INFO>
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
#include <cstdint>
#include <memory>

#include "cbu/common/defer.h"
#include "cbu/compat/atomic_ref.h"

namespace cbu {
namespace init_guard {

// Put their implementations in init_guard.cc to avoid including too many
// headers here.
void FutexWakeAll(std::uint32_t* guard) noexcept;
void FutexWakeOne(std::uint32_t* guard) noexcept;
void FutexWait(std::uint32_t* guard, std::uint32_t value) noexcept;

// After uninit, the status is modified to ABORTED
// (It's unsafe to reset it to NEW)
template <typename Values>
bool UninitNoRace(std::uint32_t* guard) noexcept {
  if (Values::IsInited(*guard)) {
    *guard = Values::ABORTED;
    return true;
  } else {
    return false;
  }
}

template <typename Values>
bool Uninit(std::uint32_t* guard) noexcept {
#ifdef CBU_SINGLE_THREADED
  return UninitNoRace<Values>(guard);
#else
  std::uint32_t v = std::atomic_ref(*guard).load(std::memory_order_acquire);
  while (Values::IsInited(v)) {
    if (std::atomic_ref(*guard).compare_exchange_weak(
            v, Values::ABORTED, std::memory_order_acquire,
            std::memory_order_acquire))
      return true;
  }
  return false;
#endif
}

#ifndef CBU_SINGLE_THREADED
template <typename Values>
bool Lock(std::uint32_t* guard, std::uint32_t v) noexcept {
  for (;;) {
    while (v == Values::NEW || v == Values::ABORTED) {
      if (std::atomic_ref(*guard).compare_exchange_weak(
              v, (v == Values::NEW) ? Values::RUNNING : Values::RUNNING_WAITING,
              std::memory_order_acquire, std::memory_order_relaxed)) {
        return true;
      }
    }

    if (Values::IsInited(v)) return false;

    if (v != Values::RUNNING_WAITING) {
      if (!std::atomic_ref(*guard).compare_exchange_strong(
              v, Values::RUNNING_WAITING, std::memory_order_acquire,
              std::memory_order_relaxed) &&
          v != Values::RUNNING_WAITING) {
        continue;
      }
    }
    FutexWait(guard, v);
    v = std::atomic_ref(*guard).load(std::memory_order_relaxed);
  }
}
#endif

void Release(std::uint32_t* guard, std::uint32_t value,
             std::uint32_t running_waiting_value) noexcept;

void Abort(std::uint32_t* guard, std::uint32_t aborted_value) noexcept;

template <typename Values>
void Release(std::uint32_t* guard, std::uint32_t value) noexcept {
  Release(guard, value, Values::RUNNING_WAITING);
}

template <typename Values>
void Abort(std::uint32_t* guard) noexcept {
  Abort(guard, Values::ABORTED);
}

template <typename Values, typename Foo, typename... Args>
inline bool InitImpl(std::uint32_t* guard, Foo&& foo, Args&&... args) noexcept(
    noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
#ifdef CBU_SINGLE_THREADED
  if (Values::IsInited(*guard)) {
    return false;
  } else {
    std::uint32_t done_value = Values::ABORTED;
    CBU_DEFER {
      if (Values::IsInited(done_value))
        *guard = done_value;
      else
        *guard = Values::ABORTED;
    };
    done_value = std::forward<Foo>(foo)(std::forward<Args>(args)...);
    return Values::IsInited(done_value);
  }
#else
  std::uint32_t v = std::atomic_ref(*guard).load(std::memory_order_relaxed);
  if (!Values::IsInited(v) && Lock<Values>(guard, v)) {
    std::uint32_t done_value = Values::ABORTED;
    CBU_DEFER {
      if (Values::IsInited(done_value))
        Release<Values>(guard, done_value);
      else
        Abort<Values>(guard);
    };
    done_value = std::forward<Foo>(foo)(std::forward<Args>(args)...);
    return Values::IsInited(done_value);
  } else {
    return false;
  }
#endif
}

}  // namespace init_guard

// Generic interface
class InitGuard {
 private:
  struct Values {
    static constexpr std::uint32_t NEW = 0;
    static constexpr std::uint32_t ABORTED = 1;
    static constexpr std::uint32_t RUNNING = 2;
    static constexpr std::uint32_t RUNNING_WAITING = 3;
    static constexpr std::uint32_t DONE = 4;

    static constexpr bool IsInited(std::uint32_t v) noexcept {
      return v == DONE;
    }
  };

 public:
  constexpr InitGuard() noexcept = default;
  InitGuard(const InitGuard&) = delete;
  InitGuard& operator=(const InitGuard&) = delete;

  // This is the normal interface you should use.
  template <typename Foo, typename... Args>
  bool init(Foo&& foo, Args&&... args) noexcept(
      noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    return init_guard::InitImpl<Values>(
        &guard_, [&]() noexcept(noexcept(
                     std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
          std::forward<Foo>(foo)(std::forward<Args>(args)...);
          return Values::DONE;
        });
  }

  // IMPORTANT: The status may be changed after this function returns
  bool inited() const noexcept {
#ifdef CBU_SINGLE_THREADED
    return inited(guard_);
#else
    return inited(std::atomic_ref(guard_).load(std::memory_order_acquire));
#endif
  }

  bool inited_no_race() const noexcept { return inited(guard_); }

  bool uninit() noexcept { return init_guard::Uninit<Values>(&guard_); }

  bool uninit_no_race() noexcept {
    return init_guard::UninitNoRace<Values>(&guard_);
  }

  static constexpr bool inited(int v) noexcept { return Values::IsInited(v); }

 private:
  std::uint32_t guard_{Values::NEW};
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
    return guard_.init(
        [&, this]() noexcept(noexcept(T(std::forward<Args>(args)...))) {
          std::construct_at(pointer(), std::forward<Args>(args)...);
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
