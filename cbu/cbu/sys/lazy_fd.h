/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021, chys <admin@CHYS.INFO>
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
#include "cbu/fsyscall/fsyscall.h"
#include "cbu/sys/init_guard.h"

namespace cbu {

// Like ScopedFD, but supports atomic lazy initialization
class LazyFD {
 public:
  explicit constexpr LazyFD(int fd = -1) noexcept
      : ig_(InitGuard::LowLevelInit(negative_to_init(fd))) {}

  ~LazyFD() noexcept {
    int fd = *ig_.raw_value_ptr();
    if (fd >= 0) ::fsys_close(fd);
  }

  LazyFD(const LazyFD&) = delete;
  LazyFD& operator=(const LazyFD&) = delete;

  constexpr int fd() const noexcept { return *ig_.raw_value_ptr(); }
  constexpr operator int() const noexcept { return fd(); }
  explicit operator bool() const = delete;

  template <typename Foo, typename... Args>
  void init(Foo&& foo, Args&&... args) noexcept(
      noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    ig_.init_with_reuse(std::forward<Foo>(foo), std::forward<Args>(args)...);
  }

 private:
  static constexpr int negative_to_init(int fd) noexcept {
    return fd >= 0 ? fd : InitGuard::INIT;
  }

  static constexpr int negative_to_aborted(int fd) noexcept {
    return fd >= 0 ? fd : InitGuard::ABORTED;
  }

 private:
  InitGuard ig_;
};

}  // namespace cbu
