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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "cbu/common/align.h"
#include "cbu/common/bit.h"
#include "cbu/malloc/pagesize.h"

namespace cbu {
namespace cbu_malloc {

#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
inline constexpr bool true_no_fail(bool) { return true; }
inline constexpr bool false_no_fail(bool) { return false; }
inline constexpr bool never_fails() { return true; }
#else
inline constexpr bool true_no_fail(bool r) { return r; }
inline constexpr bool false_no_fail(bool r) { return r; }
inline constexpr bool never_fails() { return false; }
#endif

constexpr unsigned max_category = 6;
constexpr unsigned num_categories = max_category + 1;

struct Block;
struct ThreadCategory {
  Block *free;
  unsigned count_free;
};

struct ThreadSmallCache {
  ThreadCategory category[num_categories] = {};

  void destroy() noexcept;
};

inline constexpr size_t category_to_size(unsigned n) { return (16u << n); }
inline unsigned size_to_category(size_t m) {
  assert(m != 0);
  assert(m <= category_to_size(max_category));
  unsigned n = (m - 1) | 15;
  unsigned cat = bsr(n);
  return (cat - 3);
}

inline constexpr size_t divide_by_category_size(size_t m, unsigned n) {
  return (m >> (4 + n));
}
inline constexpr size_t multiply_by_category_size(size_t m, unsigned n) {
  return (m << (4 + n));
}

static_assert(pagesize == category_to_size(max_category) * 4,
              "pagesize/max_category");
constexpr size_t small_alloc_limit = pagesize / 4;

template <typename T>
inline constexpr T pagesize_floor(T size) {
  return pow2_floor(size, pagesize);
}

template <typename T>
inline constexpr T pagesize_ceil(T size) {
  return pow2_ceil(size, pagesize);
}

constexpr size_t minimal_alignment = 2 * sizeof(void *);  // Same as ptmalloc

// Page allocators
union Page {
  char b[pagesize];
  Page *next;
};

union Description;

struct DescriptionThreadCache {
  // Description cache
  Description *desc_list = nullptr;
  unsigned desc_count = 0;
  static constexpr size_t preferred_count = 16;

  void destroy() noexcept;
};

struct PageThreadCache {
  // Cache of small pages
  static constexpr unsigned page_max_category = 7;
  static constexpr unsigned page_categories = page_max_category + 1;
  static constexpr unsigned page_preferred_count = 4;
  Page *page_list[page_categories] = {};
  unsigned char page_count[page_categories] = {};

  static constexpr unsigned size_to_page_category(size_t size) {
    return (size - pagesize) / pagesize;
  }
  static constexpr size_t page_category_to_size(unsigned cat) {
    return (unsigned(pagesize) * (cat + 1));
  }

  void destroy() noexcept;
};

constexpr uint32_t NOMERGE_LEFT = 1;
constexpr uint32_t NOMERGE_RIGHT = 2;

Page *alloc_page(size_t) noexcept;
void reclaim_page(Page *, size_t, uint32_t nomerge = 0) noexcept;
void reclaim_page(Page *, Page *, uint32_t nomerge = 0) noexcept;
bool extend_page_nomove (Page *, size_t, size_t) noexcept;

// Small allocator (with thread cache)
void *alloc_small_category (unsigned) noexcept;
void *alloc_small (size_t) noexcept;
void free_small (void *) noexcept;
void free_small(void *, size_t) noexcept;
unsigned small_allocated_category (void *) noexcept;
size_t small_allocated_size (void *) noexcept;
void small_trim(size_t) noexcept;

// Large allocator
void *alloc_large(size_t) noexcept;
void free_large (void *ptr) noexcept;
void free_large(void *ptr, size_t size) noexcept;
void *realloc_large (void *ptr, size_t newsize) noexcept;
size_t large_allocated_size (const void *) noexcept;
void large_trim(size_t) noexcept;

// Misc non-inline functions
[[noreturn]]
void memory_corrupt() noexcept;

#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
[[noreturn,gnu::cold,gnu::noinline]]
#else
[[gnu::cold,gnu::noinline]]
#endif
std::nullptr_t nomem() noexcept;

[[noreturn, gnu::cold]]
void fatal(const char* msg) noexcept;

void *mmap_wrapper(size_t) noexcept;

} // namespace cbu_malloc
} // namespace cbu
