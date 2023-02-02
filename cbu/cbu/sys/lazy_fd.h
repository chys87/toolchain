/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021-2023, chys <admin@CHYS.INFO>
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

#include <cstdint>
#include <utility>

#include "cbu/sys/init_guard.h"

namespace cbu {

// Like ScopedFD, but supports atomic lazy initialization
class LazyFD {
 private:
  struct Values {
    static constexpr std::uint32_t NEW = -1;
    static constexpr std::uint32_t ABORTED = -2;
    static constexpr std::uint32_t RUNNING = -3;
    static constexpr std::uint32_t RUNNING_WAITING = -4;

    static constexpr bool IsInited(int fd) noexcept { return fd >= 0; }
  };

 public:
  explicit constexpr LazyFD(int fd = -1) noexcept
      : guard_(fd >= 0 ? fd : Values::NEW) {}

  ~LazyFD() noexcept {
    int fd = guard_;
    if (fd >= 0) do_close(fd);
  }

  LazyFD(const LazyFD&) = delete;
  LazyFD& operator=(const LazyFD&) = delete;

  constexpr int fd() const noexcept { return guard_; }
  constexpr operator int() const noexcept { return fd(); }
  explicit operator bool() const = delete;

  template <typename Foo, typename... Args>
  bool init(Foo&& foo, Args&&... args) noexcept(
      noexcept(std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
    return init_guard::InitImpl<Values>(
        &guard_, [&]() noexcept(noexcept(
                     std::forward<Foo>(foo)(std::forward<Args>(args)...))) {
          return std::forward<Foo>(foo)(std::forward<Args>(args)...);
        });
  }

 private:
  // Do it in lazy_fd.cc so that this file doesn't need to include
  // fsyscall.h or unistd.h
  static void do_close(int fd) noexcept;

 private:
  std::uint32_t guard_;
};

}  // namespace cbu
