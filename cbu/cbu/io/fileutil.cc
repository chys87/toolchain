/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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

#include "cbu/io/fileutil.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "cbu/fsyscall/fsyscall.h"

namespace cbu {

time_t get_file_mtime(AtFile file) noexcept {
  return get_file_mtime_opt(file).value_or(0);
}

time_t get_file_mtime(int fd) noexcept {
  return get_file_mtime_opt(fd).value_or(0);
}

std::optional<time_t> get_file_mtime_opt(AtFile file) noexcept {
#ifdef STATX_MTIME
  struct statx st;
  int rc = fsys_statx(file.fd(), file.name(), 0, STATX_MTIME, &st);
  if (rc == 0) {
    return st.stx_mtime.tv_sec;
  } else {
    return std::nullopt;
  }
#else
  struct stat st;
  int rc = fsys_fstatat(file.fd(), file.name(), &st, 0);
  if (rc == 0) {
    return st.st_mtime;
  } else {
    return std::nullopt;
  }
#endif
}

std::optional<time_t> get_file_mtime_opt(int fd) noexcept {
#ifdef STATX_MTIME
  struct statx st;
  int rc = fsys_statx(fd, "", AT_EMPTY_PATH, STATX_MTIME, &st);
  if (rc == 0) {
    return st.stx_mtime.tv_sec;
  } else {
    return std::nullopt;
  }
#else
  struct stat st;
  int rc = fsys_fstat(fd, &st);
  if (rc == 0) {
    return st.st_mtime;
  } else {
    return std::nullopt;
  }
#endif
}

void touch_file(AtFile atfile, time_t timestamp) noexcept {
  struct timespec ts[2] = {{timestamp, 0}, {timestamp, 0}};
  int rv = fsys_utimensat(atfile.fd(), atfile.name(), ts, 0);
  if ((rv != 0) && fsys_errno(rv, ENOENT)) {
    // Maybe the file does not exist
    int fd = fsys_openat4(atfile.fd(), atfile.name(),
                          O_WRONLY | O_CREAT | O_CLOEXEC, 0666);
    if (fd >= 0) {
      fsys_futimens(fd, ts);
      fsys_close(fd);
    }
  }
}

void touch_file(int fd, time_t timestamp) noexcept {
  struct timespec ts[2] = {{timestamp, 0}, {timestamp, 0}};
  fsys_futimens(fd, ts);
}

bool ensure_file(AtFile atfile, mode_t mode) noexcept {
  if (fsys_faccessat(atfile.fd(), atfile.name(), F_OK, 0) == 0)
    return true;
  int fd = fsys_openat4(atfile.fd(), atfile.name(),
                        O_CREAT | O_WRONLY | O_EXCL | O_CLOEXEC, mode);
  if (fd >= 0) {
    fsys_close(fd);
    return true;
  } else {
    return false;
  }
}

}  // namespace cbu
