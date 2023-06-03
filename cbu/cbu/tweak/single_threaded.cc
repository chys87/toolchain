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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if __has_include(<cxxabi.h>)
#  include <cxxabi.h>
#endif

#include "cbu/common/procutil.h"
#include "cbu/fsyscall/fsyscall.h"

extern "C" {

[[gnu::visibility("hidden")]] char __libc_single_threaded = 1;

} // extern "C"

[[
#if defined __GNUC__ && !defined __clang__
  gnu::externally_visible,
#endif
  gnu::visibility("default"), gnu::cold]]
int pthread_create(pthread_t*, const pthread_attr_t*,
                   void *(*)(void *), void *) {
  cbu::fatal<"Creating threads in this program is disallowed">();
}

#ifdef __GLIBCXX__
static inline int __guard_test_bit(const int __byte, const int __val) {
  union {
    int __i;
    char __c[sizeof(int)];
  } __u = {0};
  __u.__c[__byte] = __val;
  return __u.__i;
}

extern "C" {

[[gnu::visibility("hidden")]] int __cxa_guard_acquire_override(int* g) noexcept
    asm("__cxa_guard_acquire");
[[gnu::visibility("hidden")]] void __cxa_guard_abort_override(int* g) noexcept
    asm("__cxa_guard_abort");
[[gnu::visibility("hidden")]] void __cxa_guard_release_override(int* g) noexcept
    asm("__cxa_guard_release");

int __cxa_guard_acquire_override(int* g) noexcept {
  if (*g == 0) {
    // Let's be lazy and don't change its value.
    // *g = _GLIBCXX_GUARD_PENDING_BIT;
    return 1;
  } else {
    return 0;
  }
}

void __cxa_guard_abort_override(int* g) noexcept {
  // *g = 0;
}

void __cxa_guard_release_override(int* g) noexcept { *g = _GLIBCXX_GUARD_BIT; }

}  // extern "C"
#endif
