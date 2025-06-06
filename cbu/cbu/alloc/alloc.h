/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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

#include <cstddef>

namespace cbu {
namespace alloc {

[[gnu::const]] bool is_alignment_valid(size_t align) noexcept;
[[gnu::const]] bool is_alignment_valid_posix(size_t align) noexcept;

struct AllocateOptions {
  // allocate/reallocte only; Maximum supported alignment is pagesize.
  // Caller's responsibility to ensure align is valid, as determined by
  // is_alignment_valid
  uint16_t align = 0;
  // allocate/allocate_page only; Not supported by reallocate
  bool zero : 1 = false;
  // Allow allocation from brk; This is only supported by allocate_page;
  // Use this only if you will directly unmap the memory rather than call
  // reclaim_page.  force_mmap always disables THP.
  bool force_mmap : 1 = false;

  template <typename Modifier>
  constexpr AllocateOptions with(Modifier modifier) noexcept {
    AllocateOptions res = *this;
    modifier(&res);
    return res;
  }

  constexpr AllocateOptions with_align(uint16_t a) noexcept {
    return with([&](AllocateOptions* o) constexpr noexcept { o->align = a; });
  }

  constexpr AllocateOptions with_zero(bool z) noexcept {
    return with([&](AllocateOptions* o) constexpr noexcept { o->zero = z; });
  }

  constexpr AllocateOptions with_force_mmap(bool m) noexcept {
    return with(
        [&](AllocateOptions* o) constexpr noexcept { o->force_mmap = m; });
  }
};


// High-level interfaces
// Supported options: align; zero
void* allocate(size_t size, AllocateOptions options) noexcept;
// Same as allocate(size, AllocateOptions{}), but optimized for the common case
void* allocate(size_t size) noexcept;
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
// You can also just call munmap to free it if force_mmap is true -- though
// this is not recommended, it can be required in certain circumstance, e.g.,
// the pages have been "gift'd" to the kernel with vmsplice SPLICE_F_GIFT
// supported options: zero; force_mmap
struct Page;
Page* allocate_page(size_t bytes, AllocateOptions options = {}) noexcept;
// Should use the same options with allocate_page for best performance
void reclaim_page(Page* page, size_t bytes,
                  AllocateOptions options = {}) noexcept;

#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
[[noreturn, gnu::cold, gnu::noinline]]
#else
[[gnu::cold, gnu::noinline]]
#endif
std::nullptr_t
nomem() noexcept;

}  // namespace alloc
}  // namespace cbu
