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

#include "cbu/sys/low_level_mutex.h"
#include <sched.h>
#include <linux/futex.h>
#include <atomic>
#include "cbu/fsyscall/fsyscall.h"

// Implement std::atomic_ref if it's missing.
// This hack is undefined behavior per C++ standard, but it works fine
// in practice
#ifndef __cpp_lib_atomic_ref
namespace std {

template <typename T>
requires (sizeof(std::atomic<T>) == sizeof(T))
inline std::atomic<T>& atomic_ref(T& val) {
  return reinterpret_cast<std::atomic<T>&>(val);
}

} // namespace std
#endif

namespace cbu {
inline namespace cbu_low_level_mutex {

void LowLevelMutex::wait(int c) noexcept {
  for (;;) {
    if ((c == 2) || ({
          int copy_a = 1;
          !std::atomic_ref(v_).compare_exchange_weak(
              copy_a, 2, std::memory_order_relaxed, std::memory_order_relaxed);})) {
      fsys_futex4(&v_, FUTEX_WAIT_PRIVATE, 2, 0);
    }

    int copy_b = 0;
    if (std::atomic_ref(v_).compare_exchange_weak(
          copy_b, 2, std::memory_order_acquire, std::memory_order_relaxed))
      break;
    c = copy_b;
  }
}

void LowLevelMutex::wake() noexcept {
  std::atomic_ref(v_).store(0, std::memory_order_release);
  fsys_futex3(&v_, FUTEX_WAKE_PRIVATE, 1);
}

void LowLevelMutex::yield() noexcept {
  if ((unsigned)std::atomic_ref(v_).load(std::memory_order_relaxed) >= 2u) {
    unlock();
    fsys_sched_yield();
    lock();
  }
}

} // namespace cbu_low_level_mutex
} // namespace cbu
