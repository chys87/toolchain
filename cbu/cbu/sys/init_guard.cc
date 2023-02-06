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

#include "cbu/sys/init_guard.h"

#include <linux/futex.h>
#include <limits>

#include "cbu/fsyscall/fsyscall.h"

namespace cbu {
namespace init_guard {

void FutexWakeAll(std::uint32_t* guard) noexcept {
  fsys_futex3(guard, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max());
}

void FutexWakeOne(std::uint32_t* guard) noexcept {
  fsys_futex3(guard, FUTEX_WAKE_PRIVATE, 1);
}

void FutexWait(std::uint32_t* guard, std::uint32_t value) noexcept {
  fsys_futex4(guard, FUTEX_WAIT_PRIVATE, value, nullptr);
}

void Release(std::uint32_t* guard, std::uint32_t value,
             std::uint32_t running_waiting_value) noexcept {
  std::uint32_t old_v =
      std::atomic_ref(*guard).exchange(value, std::memory_order_release);
  if (old_v == running_waiting_value) FutexWakeAll(guard);
}

void Abort(std::uint32_t* guard, std::uint32_t aborted_value) noexcept {
  std::atomic_ref(*guard).store(aborted_value, std::memory_order_release);
  // Only need to wake up one, which (if existent) will retry construction
  FutexWakeOne(guard);
}

}  // namespace init_guard
}  // namespace cbu
