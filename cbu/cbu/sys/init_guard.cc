/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2022, chys <admin@CHYS.INFO>
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

#include <linux/futex.h>
#include <limits>

#include "cbu/fsyscall/fsyscall.h"

namespace cbu {

bool InitGuard::uninit() noexcept {
  if (tweak::SINGLE_THREADED) return uninit_no_race();
  int v = std::atomic_ref(v_).load(std::memory_order_acquire);
  while (inited(v)) {
    if (std::atomic_ref(v_).compare_exchange_weak(
            v, ABORTED, std::memory_order_acquire, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

bool InitGuard::guard_lock(int v) noexcept {
  for (;;) {
    while (v == INIT || v == ABORTED) {
      if (std::atomic_ref(v_).compare_exchange_weak(
              v, (v == INIT) ? RUNNING : RUNNING_WAITING,
              std::memory_order_acquire, std::memory_order_relaxed)) {
        return true;
      }
    }

    if (inited(v)) return false;

    if (v != RUNNING_WAITING) {
      if (!std::atomic_ref(v_).compare_exchange_strong(
              v, RUNNING_WAITING, std::memory_order_acquire,
              std::memory_order_relaxed) &&
          v != RUNNING_WAITING) {
        continue;
      }
    }
    fsys_futex4(&v_, FUTEX_WAIT_PRIVATE, v, nullptr);
    v = std::atomic_ref(v_).load(std::memory_order_relaxed);
  }
}

void InitGuard::guard_release(int v) noexcept {
  int old_v = std::atomic_ref(v_).exchange(v, std::memory_order_release);
  if (old_v == RUNNING_WAITING)
    fsys_futex3(&v_, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max());
}

void InitGuard::guard_abort() noexcept {
  std::atomic_ref(v_).store(ABORTED, std::memory_order_release);
  // Only need to wake up one, which (if existent) will retry construction
  fsys_futex3(&v_, FUTEX_WAKE_PRIVATE, 1);
}

}  // namespace cbu
