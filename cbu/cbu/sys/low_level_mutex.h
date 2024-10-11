/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

namespace cbu {

#define CBU_MUTEX_INLINE __attribute__((__always_inline__)) inline

// For details, see Ulrich Drepper's "Futexes are Tricky"
// 0: Released
// 1: Locked; no waiter
// 2: Locked and waited for
class LowLevelMutex {
 public:
  constexpr LowLevelMutex() noexcept = default;
  LowLevelMutex(const LowLevelMutex &) = delete;
  LowLevelMutex &operator=(const LowLevelMutex &) = delete;

  CBU_MUTEX_INLINE void lock() noexcept;
  CBU_MUTEX_INLINE void unlock() noexcept;
  CBU_MUTEX_INLINE bool try_lock() noexcept;
  void yield() noexcept;

#ifndef CBU_SINGLE_THREADED
 private:
  void wait(int) noexcept;
  void wake() noexcept;

 private:
  std::uint32_t v_ = 0;
#endif
};

#if defined __x86_64__

CBU_MUTEX_INLINE void LowLevelMutex::lock() noexcept {
#  ifndef CBU_SINGLE_THREADED
  int ax = 0, one = 1;
  asm volatile(
      "\n"
      "  lock; cmpxchgl %[one], (%%rdi)\n"
#    if defined __PIC__ || defined __PIE__
      "  leaq\t1f(%%rip), %%r8\n"
#    else
      "  movl\t$1f, %%r8d\n"
#    endif
      "  jnz\tcbu_mutex_lock_wait_asm\n"  // This "function" preserves red
                                          // zone
      "1:"
      : "+a"(ax), [one] "+d"(one)
      : "D"(&v_)
      : "memory", "cc", "si", "cx", "r8", "r10", "r11");
#  endif
}

CBU_MUTEX_INLINE void LowLevelMutex::unlock() noexcept {
#  ifndef CBU_SINGLE_THREADED
  asm volatile(
      "\n"
      "  lock; decl (%%rdi)\n"
#    if defined __PIC__ || defined __PIE__
      "  leaq\t2f(%%rip), %%r8\n"
#    else
      "  mov\t$2f, %%r8d\n"
#    endif
      "  jnz\tcbu_mutex_unlock_wake_asm\n"  // This "function" preserves red
                                            // zone
      "2:\n"
      :
      : "D"(&v_)
      : "memory", "cc", "si", "ax", "dx", "cx", "r8", "r11");
#  endif
}

#else

CBU_MUTEX_INLINE void LowLevelMutex::lock() noexcept {
#  ifndef CBU_SINGLE_THREADED
  std::uint32_t copy = 0;
  if (!std::atomic_ref(v_).compare_exchange_weak(
          copy, 1, std::memory_order_acquire, std::memory_order_relaxed))
      [[unlikely]]
    wait(copy);
#  endif
}

CBU_MUTEX_INLINE void LowLevelMutex::unlock() noexcept {
#ifndef CBU_SINGLE_THREADED
  std::uint32_t c = std::atomic_ref(v_).exchange(0, std::memory_order_release);
  if (c & 2) [[unlikely]]
    wake();
#endif
}

#endif

CBU_MUTEX_INLINE bool LowLevelMutex::try_lock() noexcept {
#ifdef CBU_SINGLE_THREADED
  return true;
#else
  std::uint32_t copy = 0;
  return std::atomic_ref(v_).compare_exchange_strong(
      copy, 1, std::memory_order_acquire, std::memory_order_relaxed);
#endif
}

class SpinLock {
 public:
  constexpr SpinLock() noexcept = default;
  SpinLock(const SpinLock &) = delete;
  SpinLock &operator=(const SpinLock &) = delete;

  CBU_MUTEX_INLINE void lock() noexcept;
  CBU_MUTEX_INLINE void unlock() noexcept;
  CBU_MUTEX_INLINE bool try_lock() noexcept;
  void yield() noexcept;

 private:
  CBU_MUTEX_INLINE void pause() noexcept;

#ifndef CBU_SINGLE_THREADED
 private:
  int v_ = 0;
#endif
};

CBU_MUTEX_INLINE void SpinLock::lock() noexcept {
#ifndef CBU_SINGLE_THREADED
  do {
    while (std::atomic_ref(v_).load(std::memory_order_relaxed) != 0) {
      pause();
    }
  } while (!try_lock());
#endif
}

CBU_MUTEX_INLINE void SpinLock::unlock() noexcept {
#ifndef CBU_SINGLE_THREADED
  std::atomic_ref(v_).store(0, std::memory_order_release);
#endif
}

CBU_MUTEX_INLINE bool SpinLock::try_lock() noexcept {
#ifndef CBU_SINGLE_THREADED
  int expected = 0;
  return std::atomic_ref(v_).compare_exchange_strong(
      expected, 1, std::memory_order_acquire, std::memory_order_relaxed);
#endif
  return true;
}

CBU_MUTEX_INLINE void SpinLock::yield() noexcept {
#ifndef CBU_SINGLE_THREADED
  std::atomic_ref(v_).store(0, std::memory_order_release);
  do {
    do {
      pause();
    } while (std::atomic_ref(v_).load(std::memory_order_relaxed) != 0);
  } while (!try_lock());
#endif
}

CBU_MUTEX_INLINE void SpinLock::pause() noexcept {
#ifndef CBU_SINGLE_THREADED
#  if (defined __i386__ || defined __x86_64__) && \
      __has_builtin(__builtin_ia32_pause)
  __builtin_ia32_pause();
#  elif defined __aarch64__ && __has_builtin(__builtin_arm_yield)
  __builtin_arm_yield();
#  endif
#endif
}

#undef CBU_MUTEX_INLINE

}  // namespace cbu
