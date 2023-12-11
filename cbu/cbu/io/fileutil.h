/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include <optional>
#include <string_view>

#include "cbu/strings/zstring_view.h"

namespace cbu {

// Wraps an fd (to a directory) and a filename for openat and friends
class AtFile {
 public:
  constexpr AtFile(int f, const char *n) noexcept : fd_(f), name_(n) {}
  constexpr AtFile(const char *n = nullptr) noexcept : AtFile(AT_FDCWD, n) {}
  constexpr AtFile(const AtFile&) noexcept = default;
  constexpr AtFile& operator=(const AtFile&) noexcept = default;

  explicit constexpr operator bool() const noexcept { return name(); }
  constexpr int fd() const noexcept { return fd_; }
  constexpr const char *name() const noexcept { return name_; }

 private:
  int fd_;
  const char *name_;
};

// AtFile with known length, note that the string must still be null-terminated
class AtFileWithLength {
 public:
  constexpr AtFileWithLength(int fd, cbu::zstring_view name) noexcept
      : fd_(fd), l_(name.size()), name_(name.c_str()) {}
  constexpr AtFileWithLength(cbu::zstring_view name) noexcept
      : fd_(AT_FDCWD), l_(name.size()), name_(name.c_str()) {}
  constexpr AtFileWithLength(AtFile af) noexcept
      : fd_(af.fd()),
        l_(std::string_view(af.name()).size()),
        name_(af.name()) {}
  constexpr AtFileWithLength(const AtFileWithLength&) noexcept = default;
  constexpr AtFileWithLength& operator=(const AtFileWithLength&) noexcept = default;

  explicit constexpr operator bool() const noexcept { return name(); }
  constexpr operator AtFile() const noexcept { return {fd_, name_}; }
  constexpr int fd() const noexcept { return fd_; }
  constexpr const char* name() const noexcept { return name_; }
  constexpr size_t length() const noexcept { return l_; }
  constexpr cbu::zstring_view name_view() const noexcept { return {name_, l_}; }

 private:
  int fd_;
  unsigned int l_;
  const char* name_;
};

// If anything is wrong, returns 0
time_t get_file_mtime(AtFile) noexcept;
time_t get_file_mtime(int) noexcept;

// If anything is wrong, returns nullopt
std::optional<time_t> get_file_mtime_opt(AtFile) noexcept;
std::optional<time_t> get_file_mtime_opt(int) noexcept;

void touch_file(AtFile, time_t) noexcept;
void touch_file(int, time_t) noexcept;
void touch_file(AtFile) noexcept;
void touch_file(int) noexcept;

// If a file doesn't exist, create it.
bool ensure_file(AtFile, mode_t) noexcept;

// Utility functions for conversion between iovec and string_view
inline constexpr iovec sv2iov(std::string_view sv) noexcept {
  return {const_cast<char *>(sv.data()), sv.length()};
}

inline constexpr std::string_view iov2sv(iovec v) noexcept {
  return {static_cast<const char *>(v.iov_base), v.iov_len};
}

}  // namespace cbu
