/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <type_traits>

#include "cbu/alloc/pagesize.h"
#include "cbu/common/bit.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
namespace alloc {

#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
inline constexpr bool true_no_fail(bool) { return true; }
inline constexpr bool false_no_fail(bool) { return false; }
inline constexpr bool never_fails() { return true; }
#else
inline constexpr bool true_no_fail(bool r) { return r; }
inline constexpr bool false_no_fail(bool r) { return r; }
inline constexpr bool never_fails() { return false; }
#endif

constexpr unsigned kMaxCategory = 6;
constexpr unsigned kNumCategories = kMaxCategory + 1;

constexpr size_t category_to_size(unsigned n) noexcept { return (16u << n); }

constexpr unsigned size_to_category(size_t m) noexcept {
  assert(m != 0);
  assert(m <= category_to_size(kMaxCategory));
  unsigned n = (m - 1) | 15;
  unsigned cat = bsr(n);
  return (cat - 3);
}

constexpr size_t divide_by_category_size(size_t m, unsigned n) noexcept {
  return (m >> (4 + n));
}

constexpr size_t multiply_by_category_size(size_t m, unsigned n) noexcept {
  return (m << (4 + n));
}

static_assert(kPageSize == category_to_size(kMaxCategory) * 4,
              "kPageSize/kMaxCategory");
constexpr size_t kSmallAllocLimit = kPageSize / 4;

template <typename T>
inline constexpr T pagesize_floor(T size) noexcept {
  return pow2_floor(size, kPageSize);
}

template <typename T>
inline constexpr T pagesize_ceil(T size) noexcept {
  return pow2_ceil(size, kPageSize);
}

// Page allocators
struct Page {
  union {
    alignas(kPageSize) char b[kPageSize];
    Page* next;
  };
};

static_assert(sizeof(Page) == kPageSize);
static_assert(alignof(Page) == kPageSize);

constexpr uint32_t RECLAIM_PAGE_NOMERGE_LEFT = 1;
constexpr uint32_t RECLAIM_PAGE_NOMERGE_RIGHT = 2;
constexpr uint32_t RECLAIM_PAGE_NO_THP = 4;
// Use this if the caller knows the page is clean
constexpr uint32_t RECLAIM_PAGE_CLEAN = 8;

void reclaim_page(Page*, size_t, uint32_t option_bitmask) noexcept;
bool extend_page_nomove(Page*, size_t, size_t) noexcept;

// Small allocator (with thread cache)
void* alloc_small_category(unsigned) noexcept;
void* alloc_small(size_t) noexcept;
void free_small(void*) noexcept;
void free_small(void*, size_t) noexcept;
unsigned small_allocated_category(void*) noexcept;
size_t small_allocated_size(void*) noexcept;
void small_trim(size_t) noexcept;

// Large allocator
void* alloc_large(size_t size, bool zero) noexcept;
void free_large(void* ptr) noexcept;
void free_large(void* ptr, size_t size) noexcept;
void* realloc_large(void* ptr, size_t newsize) noexcept;
size_t large_allocated_size(const void*) noexcept;
void large_trim(size_t) noexcept;

// Raw page allocation
// No corresponding deallocation is provided.  Caller should either keep
// the memory or munmap it (or in case of brk, use MADV_DONTNEED).
// This allocator does do some caching to reduce the number of mmap calls
// and reduce defragmentation.
// This allocator guarantees returned pages are zero initialized
class RawPageAllocator {
 public:
  constexpr RawPageAllocator(bool use_brk, bool allow_thp) noexcept
      : use_brk_(use_brk), allow_thp_(use_brk || allow_thp) {}

  Page* allocate(size_t size) noexcept;

  static bool is_from_brk(void* ptr) noexcept;

  constexpr bool use_brk() const noexcept { return use_brk_; }
  constexpr bool allow_thp() const noexcept { return allow_thp_; }

  static RawPageAllocator instance_brk;
  static RawPageAllocator instance_mmap;
  static RawPageAllocator instance_mmap_no_thp;

 private:
  struct CachedPage;

  CachedPage* cached_page_ = nullptr;
  [[no_unique_address]] LowLevelMutex lock_;
  bool use_brk_;
  bool allow_thp_;
};

inline constinit RawPageAllocator RawPageAllocator::instance_brk{true, true};
inline constinit RawPageAllocator RawPageAllocator::instance_mmap{false, true};
inline constinit RawPageAllocator RawPageAllocator::instance_mmap_no_thp{false,
                                                                         false};

template <typename T>
T load_acquire(T* ptr) noexcept {
#ifdef CBU_SINGLE_THREADED
  return *ptr;
#else
  return std::atomic_ref(*ptr).load(std::memory_order_acquire);
#endif
}

template <typename T>
void store_release(T* ptr, std::type_identity_t<T> value) noexcept {
#ifdef CBU_SINGLE_THREADED
  *ptr = value;
#else
  std::atomic_ref(*ptr).store(value, std::memory_order_release);
#endif
}

}  // namespace alloc
}  // namespace cbu
