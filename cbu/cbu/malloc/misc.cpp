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

#include "cbu/malloc/private.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mutex>
#include "cbu/common/fastarith.h"
#include "cbu/fsyscall/fsyscall.h"
#include "cbu/malloc/pagesize.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
inline namespace cbu_malloc {

void fatal(const char* s) noexcept {
  size_t n = write(2, s, strlen(s));
  (void)n;
  abort();
}

void memory_corrupt() noexcept {
  fatal("Memory corrupt\n");
}

std::nullptr_t nomem() noexcept {
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
  fatal("Insufficient memory\n");
#else
  errno = ENOMEM;
  return nullptr;
#endif
}

namespace {

class MmapWrapper {
 public:
  void *alloc(size_t size) noexcept;

 private:
  LowLevelMutex mutex_ {};
  bool hint_downward_ = false;
  uintptr_t hint_ptr_ = 0;

  static constexpr int kFlags = MAP_PRIVATE | MAP_ANONYMOUS;
};

void *MmapWrapper::alloc(size_t size) noexcept {
  assert(size > 0);
  assert(size < 0x7fffffff);
  assert(size % pagesize == 0);
  if (hugepagesize && (size % hugepagesize == 0)) {
    void *p;
    {
      std::lock_guard locker(mutex_);

      uintptr_t hint = hint_ptr_;
      if (hint_downward_) {
        if (cbu::sub_overflow(&hint, uintptr_t(size)))
          hint = 0;
      } else {
        uintptr_t hint_alloc_end;
        if (cbu::add_overflow(hint, uintptr_t(size), &hint_alloc_end))
          hint = 0;
      }

      p = fsys_mmap(reinterpret_cast<void *>(static_cast<uintptr_t>(hint)), size, PROT_READ | PROT_WRITE, kFlags, -1, 0);
      if (false_no_fail(fsys_mmap_failed(p)))
        return nullptr;
      if (uintptr_t(p) != hint)
        hint_downward_ = (uintptr_t(p) < hint);

      uintptr_t next_hint = reinterpret_cast<uintptr_t>(p);
      if (!hint_downward_)
        next_hint += size + hugepagesize - 1;
      next_hint &= ~uintptr_t(hugepagesize - 1);

      hint_ptr_ = next_hint;
    }
    if (uintptr_t(p) % hugepagesize == 0)
      fsys_madvise(p, size, MADV_HUGEPAGE);
    return p;
  } else {
    return fsys_mmap(nullptr, size, PROT_READ | PROT_WRITE, kFlags, -1, 0);
  }
}

} // namespace

void* mmap_wrapper(size_t size) noexcept {
  static MmapWrapper wrapper;
  return wrapper.alloc(size);
}

} // namespace cbu_malloc
} // namespace cbu
