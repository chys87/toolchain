/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

#include "cbu/malloc/permanent.h"
#include <sys/mman.h>
#include <unistd.h>
#include "cbu/common/byte_size.h"
#include "cbu/fsyscall/fsyscall.h"
#include "cbu/malloc/private.h"

namespace cbu {
namespace malloc_details {
namespace {

constexpr size_t pseudo_hugepagesize =
  hugepagesize ? hugepagesize : 16 * pagesize;

inline void *linux_brk(void *ptr) {
#if FSYSCALL_USE
  return fsys_generic(__NR_brk, void *, 1, ptr);
#else
  return (void *) syscall(__NR_brk, ptr);
#endif
}

void *get_perma_pages_brk(size_t size) noexcept {
  static LowLevelMutex lock {};
  static void *heap_avail = nullptr;
  static void *heap_end = nullptr;

  std::lock_guard locker(lock);

  void *ret_start = heap_avail;
  void *ret_end = byte_advance(ret_start, size);

  if (ret_end <= heap_end) {
    heap_avail = ret_end;
    return ret_start;
  }

  static bool brk_bad = false;
  if (brk_bad)
    return nullptr;

  // Try to take advantage of transparent huge pages
  // But don't do this if we don't need much memory
  // Therefore, we return a value larger than hugepagesize from the second call on

  void *old_alloc_end = heap_end;
  if (ret_start == nullptr) {
    void *initial_brk = linux_brk(nullptr);
    ret_start = old_alloc_end = initial_brk;
    ret_end = byte_advance(ret_start, size);
  }

  void *alloc_end = cbu::pow2_ceil(ret_end, pseudo_hugepagesize);

  void *new_brk_end = linux_brk(alloc_end);
  heap_end = new_brk_end;
  if (new_brk_end < alloc_end) {
    brk_bad = true;
    return nullptr;
  }

  heap_avail = ret_end;

  return ret_start;
}

struct CachedPage {
  CachedPage *next;
  unsigned size;
};

void *get_perma_pages_mmap(size_t size) noexcept {
  static LowLevelMutex lock {};
  static CachedPage *cached = nullptr;

  if (std::lock_guard locker(lock); cached) {
    CachedPage **prev = &cached;
    CachedPage *cur = cached;
    while (cur) {
      unsigned cur_size = cur->size;
      CachedPage *cur_next = cur->next;
      if (cur_size > size) {
        CachedPage *alloc_end = byte_advance(cur, size);
        alloc_end->next = cur_next;
        alloc_end->size = cur_size - size;
        *prev = alloc_end;
        return cur;
      } else if (cur_size == size) {
        *prev = cur_next;
        return cur;
      } else {
        prev = &cur->next;
        cur = cur_next;
      }
    }
  }

  size_t alloc_size = cbu::pow2_ceil(size, pseudo_hugepagesize);
  void *np = raw_mmap_alloc(alloc_size);
  if (false_no_fail(np == nullptr))
    return nullptr;

  if (alloc_size == size)
    return np;

  std::lock_guard locker(lock);
  CachedPage *remaining = static_cast<CachedPage *>(byte_advance(np, size));
  remaining->next = cached;
  remaining->size = alloc_size - size;
  cached = remaining;

  return np;
}

} // namespace

void *get_pages_for_perma(size_t size) noexcept {
  void *ptr = get_perma_pages_brk(size);
  if (ptr == nullptr)
    ptr = get_perma_pages_mmap(size);
  return ptr;
}

}  // namespace malloc_details
}  // namespace cbu
