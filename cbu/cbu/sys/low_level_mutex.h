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

#include <atomic>
#include "cbu/compat/atomic_ref.h"
#include "cbu/tweak/tweak.h"

namespace cbu {
inline namespace cbu_low_level_mutex {

#define CBU_MUTEX_INLINE __attribute__((__always_inline__)) inline

// For details, see Ulrich Drepper's "Futexes are Tricky"
// 0: Released
// 1: Locked; no waiter
// 2: Locked and waited for
class LowLevelMutex {
public:
  constexpr LowLevelMutex() noexcept = default;
  LowLevelMutex(const LowLevelMutex &) = delete;
  LowLevelMutex &operator = (const LowLevelMutex &) = delete;

  CBU_MUTEX_INLINE void lock() noexcept;
  CBU_MUTEX_INLINE void unlock() noexcept;
  CBU_MUTEX_INLINE bool try_lock() noexcept;
  void yield() noexcept;

private:
  void wait(int) noexcept;
  void wake() noexcept;

private:
  int v_ = 0;
};

#if defined __x86_64__

CBU_MUTEX_INLINE void LowLevelMutex::lock() noexcept {
  if (!tweak::SINGLE_THREADED) {
    int ax = 0, one = 1;
    asm volatile ("\n"
      "  lock; cmpxchgl %[one], (%%rdi)\n"
#if defined __PIC__ || defined __PIE__
      "  leaq\t1f(%%rip), %%r8\n"
#else
      "  movl\t$1f, %%r8d\n"
#endif
      "  jnz\tcbu_mutex_lock_wait_asm\n" // This "function" preserves red zone
      "1:"
      : "+a"(ax), [one]"+d"(one)
      : "D"(&v_)
      : "memory", "cc", "si", "cx", "r8", "r10", "r11"
      );
  }
}

CBU_MUTEX_INLINE void LowLevelMutex::unlock() noexcept {
  if (!tweak::SINGLE_THREADED) {
    asm volatile ("\n"
      "  lock; decl (%%rdi)\n"
#if defined __PIC__ || defined __PIE__
      "  leaq\t2f(%%rip), %%r8\n"
#else
      "  mov\t$2f, %%r8d\n"
#endif
      "  jnz\tcbu_mutex_unlock_wake_asm\n" // This "function" preserves red zone
      "2:\n"
      :
      : "D"(&v_)
      : "memory", "cc", "si", "ax", "dx", "cx", "r8", "r11");
  }
}

#else

CBU_MUTEX_INLINE void LowLevelMutex::lock() noexcept {
  if (!tweak::SINGLE_THREADED) {
    int copy = 0;
    if (!std::atomic_ref(v_).compare_exchange_weak(
        copy, 1, std::memory_order_acquire, std::memory_order_relaxed))
      wait(copy);
  }
}

CBU_MUTEX_INLINE void LowLevelMutex::unlock() noexcept {
  if (!tweak::SINGLE_THREADED) {
    int c = std::atomic_ref(v_).fetch_sub(1, std::memory_order_release) - 1;
    if (__builtin_expect(c, 0) != 0)
      wake();
  }
}

#endif

CBU_MUTEX_INLINE bool LowLevelMutex::try_lock() noexcept {
  if (tweak::SINGLE_THREADED) {
    return true;
  } else {
    int copy = 0;
    return std::atomic_ref(v_).compare_exchange_strong(
        copy, 1, std::memory_order_acquire, std::memory_order_relaxed);
  }
}

// For compatibility only
using LowLevelTmMutex = LowLevelMutex;

#undef CBU_MUTEX_INLINE

} // namesapce cbu_low_level_mutex
} // namespace cbu
