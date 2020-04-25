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

#include "cbu/fsyscall/fsyscall.h"
#include <unistd.h>
#include <utility>

namespace cbu {
inline namespace cbu_scoped_fd {

class ScopedFD {
 public:
  explicit constexpr ScopedFD(int fd = -1) noexcept : fd_(fd) {}
  ScopedFD(const ScopedFD &) = delete;
  ScopedFD(ScopedFD &&other) noexcept : fd_(other.release()) {}
  ~ScopedFD() noexcept { if (fd_ >= 0) ::fsys_close(fd_); }

  ScopedFD &operator = (const ScopedFD &) = delete;
  ScopedFD &operator = (ScopedFD &&other) noexcept {
    swap(other); return *this;
  }

  void reset(int fd = -1) noexcept {
    int fdo = std::exchange(fd_, fd);
    if (fdo >= 0)
      ::fsys_close(fdo);
  }
  void reset_unsafe(int fd) noexcept { fd_ = fd; }

  void swap(ScopedFD &other) noexcept { std::swap(fd_, other.fd_); }
  void close() noexcept { reset(-1); }

  constexpr int fd() const noexcept { return fd_; }
  constexpr operator int() const noexcept { return fd_; }
  operator bool() const = delete;

  int release() noexcept { return std::exchange(fd_, -1); }

 public:
  static constexpr inline bool bitwise_movable(ScopedFD *) { return true; }

 private:
  int fd_;
};

} // inline namespace cbu_scoped_fd
} // namespace cbu
