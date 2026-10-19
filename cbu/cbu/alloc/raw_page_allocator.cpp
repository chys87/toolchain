/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
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

#include <string.h>
#include <sys/mman.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/private/common.h"
#include "cbu/common/bit.h"
#include "cbu/common/byte_size.h"
#include "cbu/common/hint.h"
#include "cbu/fsyscall/fsyscall.h"

namespace cbu {
namespace alloc {
namespace {

LowLevelMutex brk_mutex;
// The brk region is [brk_initial, brk_cur); brk_cur only grows.
// brk_initial is always published before brk_cur's first non-zero value.
std::atomic<uintptr_t> brk_initial{0};
std::atomic<uintptr_t> brk_cur{0};

#if FSYSCALL_USE
inline void* linux_brk(void* ptr) {
  return fsys_generic(__NR_brk, void*, 1, ptr);
}
#else
inline void* linux_brk(void* ptr) {
  return reinterpret_cast<void*>(syscall(__NR_brk, ptr));
}
#endif

void* raw_brk_pages(size_t size, size_t* alloc_size) noexcept {
  std::lock_guard locker(brk_mutex);
  uintptr_t cur = brk_cur.load(std::memory_order_relaxed);
  if (cur == 0) {
    cur = uintptr_t(cbu::pow2_ceil(linux_brk(nullptr), kPageSize));
    brk_initial.store(cur, std::memory_order_relaxed);
    brk_cur.store(cur, std::memory_order_release);
  }
  size_t preferred_size =
      std::max<size_t>(size, kTHPSize ? kTHPSize : 32 * kPageSize);
  uintptr_t target = cur + preferred_size;
  if constexpr (kTHPSize > 0) target = cbu::pow2_ceil(target, kTHPSize);
  *alloc_size = target - cur;
  void* brk_new = linux_brk(reinterpret_cast<void*>(target));
  if (uintptr_t(brk_new) != target) return nullptr;
  brk_cur.store(target, std::memory_order_release);
  return CBU_HINT_NONNULL(reinterpret_cast<void*>(cur));
}

void* raw_mmap_pages(size_t size, bool allow_thp) noexcept {
  void* p = fsys_mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (false_no_fail(fsys_mmap_failed(p))) {
    p = nullptr;
  } else {
#ifdef MADV_NOHUGEPAGE
    if (kTHPSize && !allow_thp) fsys_madvise(p, size, MADV_NOHUGEPAGE);
#endif
  }
  return p;
}

}  // namespace

struct RawPageAllocator::CachedPage {
  CachedPage* next;
  unsigned size;
};

Page* RawPageAllocator::allocate(size_t size) noexcept {
  // Let's just run the whole function while holding the lock, even when
  // we're calling mmap.
  // Memmory mapping is inherently not parallelizable, so this really is no
  // penalty.  It also significantly simplifies our handling of brk.
  std::lock_guard locker(lock_);

  if (cached_page_) {
    CachedPage **prev = &cached_page_;
    CachedPage *cur = cached_page_;
    while (cur) {
      size_t cur_size = cur->size;
      CachedPage *cur_next = cur->next;
      if (cur_size > size) {
        CachedPage *alloc_end = byte_advance(cur, size);
        alloc_end->next = cur_next;
        alloc_end->size = cur_size - size;
        *prev = alloc_end;
        memset(cur, 0, sizeof(*cur));  // Zero it before returning
        return reinterpret_cast<Page*>(cur);
      } else if (cur_size == size) {
        *prev = cur_next;
        memset(cur, 0, sizeof(*cur));  // Zero it before returning
        return reinterpret_cast<Page*>(cur);
      } else {
        prev = &cur->next;
        cur = cur_next;
      }
    }
  }

  void* np;
  size_t alloc_size;

  if (use_brk_) {
    np = raw_brk_pages(size, &alloc_size);
    if (np == nullptr) return nullptr;
  } else {
    bool use_thp = kTHPSize && allow_thp_;
    alloc_size = cbu::pow2_ceil(size, kTHPSize ? kTHPSize : 32 * kPageSize);

    np = raw_mmap_pages(alloc_size, use_thp);
    if (false_no_fail(np == nullptr)) return nullptr;
  }

  if (alloc_size > size) {
    CachedPage* remaining = static_cast<CachedPage*>(byte_advance(np, size));
    remaining->next = cached_page_;
    remaining->size = alloc_size - size;
    cached_page_ = remaining;
  }

  return static_cast<Page*>(np);
}

bool RawPageAllocator::is_from_brk(void* ptr) noexcept {
#ifdef CBU_NO_BRK
  return false;
#else
  // Load the end first: whenever it is non-zero, the initial value must
  // already have been published before it, so the pair is consistent.
  // If it is still zero, the comparison below is false anyway.
  uintptr_t end = brk_cur.load(std::memory_order_acquire);
  uintptr_t begin = brk_initial.load(std::memory_order_acquire);
  uintptr_t p = uintptr_t(ptr);
  return p >= begin && p < end;
#endif
}

}  // namespace alloc
}  // namespace cbu
