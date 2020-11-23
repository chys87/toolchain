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

#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <sys/uio.h>
#include <string_view>

namespace cbu {
inline namespace cbu_fileutil {

// Wraps an fd (to a directory) and a filename for openat and friends
class AtFile {
 public:
#ifndef __clang__
  constexpr
#endif
  AtFile(const char *n = nullptr) noexcept : AtFile(AT_FDCWD, n) {}
  AtFile(const AtFile &) noexcept = default;

  explicit constexpr operator bool() const noexcept { return name(); }
  constexpr int fd() const noexcept { return fd_; }

// On x86-64, pointers that are nominally 64-bit actually have only 48
// significant bits.  We take advantage of this and try to save the fd and
// pointer to 64 bits.
// However, that would imply that valid fds are < 32768, which is not always
// the case.
#if defined __linux__ && defined __x86_64__ && defined __LP64__ && \
    AT_FDCWD < 0 && AT_FDCWD >= -32768 && !defined CBU_LARGE_FD
 public:
#ifndef __clang__
  constexpr
#endif
  AtFile(int f, const char *n) noexcept :
    fd_(f), name_u_(reinterpret_cast<uint64_t>(n)) {}
#ifndef __clang__
  constexpr
#endif
  const char *name() const noexcept {
    return reinterpret_cast<const char *>(name_u_);
  }

 private:
  int fd_ : 16;
  uint64_t name_u_ : 48;
#else
 public:
  constexpr AtFile(int f, const char *n) noexcept : fd_(f), name_(n) {}
  constexpr const char *name() const noexcept { return name_; }

 private:
  int fd_;
  const char *name_;
#endif
};

// If anything is wrong, returns 0
time_t get_file_mtime(AtFile) noexcept;
time_t get_file_mtime(int) noexcept;

void touch_file(AtFile, time_t) noexcept;
void touch_file(int, time_t) noexcept;

// Utility functions for conversion between iovec and string_view
inline constexpr iovec sv2iov(std::string_view sv) noexcept {
  return {const_cast<char *>(sv.data()), sv.length()};
}

inline constexpr std::string_view iov2sv(iovec v) noexcept {
  return {static_cast<const char *>(v.iov_base), v.iov_len};
}

} // namespace cbu_fileutil
} // namepsace cbu
