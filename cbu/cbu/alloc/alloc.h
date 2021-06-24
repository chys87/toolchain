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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "cbu/alloc/pagesize.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
namespace alloc {

struct AllocateOptions {
  // allocate/reallocte only; Maximum supported alignment is pagesize
  uint16_t align = 0;
  // Almost all interfaces support this
  bool zero : 1 = false;
  // By default, we try to take advantage of transparent huge pages if supported
  // by the kernel, but in certain cases we may want to disable it, e.g.
  // allocating for buffer for use with vmsplice.
  bool allow_thp : 1 = true;

  template <typename Modifier>
  constexpr AllocateOptions with(Modifier modifier) noexcept {
    AllocateOptions res = *this;
    modifier(&res);
    return res;
  }

  constexpr AllocateOptions with_align(uint16_t a) noexcept {
    return with([&](AllocateOptions* o) noexcept { o->align = a; });
  }

  constexpr AllocateOptions with_zero(bool z) noexcept {
    return with([&](AllocateOptions* o) noexcept { o->zero = z; });
  }

  constexpr AllocateOptions with_thp(bool h) noexcept {
    return with([&](AllocateOptions* o) noexcept { o->allow_thp = h; });
  }
};


// High-level interfaces
// Supported options: align; zero
void* allocate(size_t size, AllocateOptions options = {}) noexcept;
// Supported options: align
[[gnu::noinline]] void* reallocate(void* ptr, size_t new_size,
                 AllocateOptions options = {}) noexcept;
[[gnu::noinline]] void reclaim(void* ptr) noexcept;
[[gnu::noinline]] void reclaim(void* ptr, size_t size) noexcept;
[[gnu::noinline]] size_t allocated_size(void* ptr) noexcept;
void trim(size_t pad) noexcept;

// Low-level interface -- page allocation
// Allocating pages is like anonymous mmap (however, if you prefer the memory
// to be zero'd you need to set options.zero to true).
// Like mmap/munmap, and unlikely malloc/free, allocate_page and reclaim_page do
// not need to be called in pairs.  You can well allocate two pages with
// allocate_page, and reclaim just one of them with reclaim_page.
// You can also just call munmap to free it -- though this is not recommended,
// it can be required in certain circumstance, e.g., the pages have been
// "gift'd" to the kernel with vmsplice SPLICE_F_GIFT
// supported options: allow_thp
struct Page;
Page* allocate_page(size_t bytes, AllocateOptions options = {}) noexcept;
// Should use the same options with allocate_page for best performance
void reclaim_page(Page* page, size_t bytes,
                  AllocateOptions options = {}) noexcept;

// Lower-level interface -- raw page allocation
// No corresponding deallocation is provided.  Caller should either keep
// the memory or munmap it.
// This allocator does do some caching to reduce the number of mmap calls
// and reduce defragmentation.
// This allocator also guarantees returned pages are zero initialized
class RawPageAllocator {
 public:
  constexpr RawPageAllocator(bool allow_thp) noexcept
      : allow_thp_(allow_thp) {}

  Page* allocate(size_t size) noexcept;

  // Just allocate pages with mmap
  // The returned memory is guaranteed to be zero initialized
  static void* raw_mmap_pages(size_t size, bool allow_thp = true,
                              void* hint = nullptr) noexcept;

  template <bool AllowThp, typename... Tags>
  static RawPageAllocator instance;

 private:
  struct CachedPage;

  CachedPage* cached_page_ = nullptr;
  LowLevelMutex lock_;
  bool allow_thp_;
  bool hint_downward_ = false;
  void* hint_address_ = nullptr;
};

template <bool AllowThp, typename... Tags>
inline constinit RawPageAllocator RawPageAllocator::instance{AllowThp};

// Misc functions, intended for use by wrappers (e.g. cbu/malloc) to provide
// consistent error messages.
[[noreturn]] void memory_corrupt() noexcept;

#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
[[noreturn, gnu::cold, gnu::noinline]]
#else
[[gnu::cold, gnu::noinline]]
#endif
std::nullptr_t
nomem() noexcept;

[[noreturn, gnu::cold]] void fatal(const char* msg) noexcept;

}  // namespace alloc
}  // namespace cbu
