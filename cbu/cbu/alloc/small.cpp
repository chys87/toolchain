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

#include "cbu/alloc/private/small.h"

#include <atomic>
#include <mutex>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/private/common.h"
#include "cbu/alloc/private/tc.h"
#include "cbu/common/byte_size.h"
#include "cbu/common/procutil.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
namespace alloc {
namespace {

// Allocated on a page
struct Run {
  unsigned cat;
  unsigned allocated;
};

static_assert(sizeof(Block) <= category_to_size(0), "Block too large");
static_assert(sizeof(Run) <= category_to_size(0), "Run too large");

inline Run* block2run(Block* ptr) {
  return (Run*)pagesize_floor(static_cast<void*>(ptr));
}

void free_small_list(Block* ptr) {
  while (ptr) {
    Block* p = std::exchange(ptr, ptr->next);

    Run* run = block2run(p);
    auto count = p->count;
    int remain =
#ifdef CBU_SINGLE_THREADED
        (run->allocated -= count)
#else
        std::atomic_ref(run->allocated)
            .fetch_sub(count, std::memory_order_release) -
        count
#endif
        ;
    if (remain <= 0) {
      if (remain < 0)
        fatal<"Memory corrupt: run->allocated < 0 at free_small">();
      reclaim_page((Page*)run, kPageSize);
    }
  }
}

// Use fallback_cache when thread_cache is unusable
constinit SmallCache fallback_cache{};
#ifndef CBU_SINGLE_THREADED
constinit LowLevelMutex fallback_cache_lock{};
#endif

void* alloc_small_category_with_cache(SmallCache* cache, unsigned cat) {
  ThreadCategory* catp = &cache->category[cat];
  if (Block* free = catp->free) {
    catp->count_free--;
    unsigned remaining = --(free->count);
    Block* p = byte_advance(free, multiply_by_category_size(remaining, cat));
    if (remaining == 0) catp->free = free->next;
    return p;
  }

  Run* run = (Run*)allocate_page(kPageSize);
  if (false_no_fail(run == nullptr)) return nullptr;
  unsigned cap = divide_by_category_size(kPageSize, cat) - 1;
  run->cat = cat;
  run->allocated = cap;

  Block* p = byte_advance((Block*)run, category_to_size(cat));
  Block* np = byte_advance(p, category_to_size(cat));
  np->next = nullptr;
  np->count = cap - 1;
  catp->free = np;
  catp->count_free = cap - 1;
  return p;
}

void free_small_with_cache(SmallCache* cache, void* ptr) {
  Block* p = static_cast<Block*>(ptr);

  unsigned cat = block2run(p)->cat;
  if (cat > kMaxCategory)
    fatal<"Memory corrupt: category invalid at free_small">();

  if (uintptr_t(p) & (category_to_size(cat) - 1))
    fatal<"Memory corrupt: alignment invalid at free_small">();

  ThreadCategory* catp = &cache->category[cat];
  p->count = 1;
  if (catp->count_free >= 256) {
    p->next = nullptr;
    catp->count_free = 1;
    free_small_list(std::exchange(catp->free, p));
  } else {
    p->next = catp->free;
    catp->free = p;
    catp->count_free++;
  }
}

}  // namespace

void SmallCache::clear() noexcept {
  for (ThreadCategory& catg : category) {
    catg.count_free = 0;
    free_small_list(std::exchange(catg.free, nullptr));
  }
}

void* alloc_small_category(unsigned cat) noexcept {
#ifdef CBU_SINGLE_THREADED
  return alloc_small_category_with_cache(&fallback_cache, cat);
#else
  if (ThreadCache* tc = get_or_create_thread_cache()) {
    return alloc_small_category_with_cache(&tc->small_cache, cat);
  } else {
    std::lock_guard locker(fallback_cache_lock);
    return alloc_small_category_with_cache(&fallback_cache, cat);
  }
#endif
}

void* alloc_small(size_t size) noexcept {
  return alloc_small_category(size_to_category(size));
}

void free_small(void* ptr) noexcept {
#ifdef CBU_SINGLE_THREADED
  free_small_with_cache(&fallback_cache, ptr);
#else
  if (ThreadCache* tc = get_or_create_thread_cache()) {
    free_small_with_cache(&tc->small_cache, ptr);
  } else {
    std::lock_guard locker(fallback_cache_lock);
    free_small_with_cache(&fallback_cache, ptr);
  }
#endif
}

void free_small(void* ptr, size_t size) noexcept {
  if (size > small_allocated_size(ptr))
    fatal<"Memory corrupt: size invalid at free_small">();
  return free_small(ptr);
}

unsigned small_allocated_category(void* ptr) noexcept {
  Block* p = static_cast<Block*>(ptr);
  Run* run = block2run(p);
  return run->cat;
}

size_t small_allocated_size(void* ptr) noexcept {
  return category_to_size(small_allocated_category(ptr));
}

void small_trim(size_t) noexcept {
#ifdef CBU_SINGLE_THREADED
  fallback_cache.clear();
#else
  if (ThreadCache* tc = get_thread_cache()) tc->small_cache.clear();
  {
    std::lock_guard locker(fallback_cache_lock);
    fallback_cache.clear();
  }
#endif
}

}  // namespace alloc
}  // namespace cbu
