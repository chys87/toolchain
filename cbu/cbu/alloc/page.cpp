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

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <mutex>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/private.h"
#include "cbu/alloc/rb.h"
#include "cbu/alloc/tc.h"
#include "cbu/alloc/trie.h"
#include "cbu/common/byte_size.h"
#include "cbu/common/hint.h"
#include "cbu/fsyscall/fsyscall.h"

namespace cbu {
namespace alloc {
namespace {

union Description {
  struct {
    RbLink<Description> link_ad;
    RbLink<Description> link_szad;
    Page* addr;
    size_t size;
  };
  struct {
    Description* next;
    unsigned count;
  };
};

struct DescriptionCache {
  // Description cache
  Description* desc_list = nullptr;
  unsigned desc_count = 0;
  static constexpr size_t kPreferredCount = 16;

  void clear() noexcept;
};

CachePool<DescriptionCache> description_cache_pool;

PermaAlloc<Description> description_allocator;

Description* alloc_description() {
  UniqueCache unique_cache(&description_cache_pool);
  if (!unique_cache) return description_allocator.alloc();

  DescriptionCache& description_cache = *unique_cache;

  Description* node = description_cache.desc_list;
  if (node == nullptr) {
    constexpr size_t kPreferredCount = DescriptionCache::kPreferredCount;
    node = description_allocator.alloc_list(kPreferredCount);
    if (false_no_fail(!node)) return nullptr;
    description_cache.desc_count = kPreferredCount;
  }
  description_cache.desc_count--;
  if (node->count > 1) {
    Description* rnode = node + 1;
    rnode->next = node->next;
    rnode->count = node->count - 1;
    description_cache.desc_list = rnode;
  } else {
    description_cache.desc_list = node->next;
  }
  return node;
}

void free_description(Description* desc) {
  UniqueCache unique_cache(&description_cache_pool);
  if (!unique_cache) {
    description_allocator.free(desc);
    return;
  }

  DescriptionCache& description_cache = *unique_cache;

  if (description_cache.desc_count >=
      DescriptionCache::kPreferredCount * 4 - 1) {
    description_allocator.free_list(description_cache.desc_list);
    description_cache.desc_list = nullptr;
    description_cache.desc_count = 0;
  }

  desc->next = description_cache.desc_list;
  desc->count = 1;
  description_cache.desc_count++;
  description_cache.desc_list = desc;
}

struct DescriptionAdRbAccessor {
  using Node = Description;
  static constexpr auto link = &Node::link_ad;

  static int cmp(const Page* a, const Node* b) noexcept {
    if (a < b->addr)
      return -1;
    else if (a == b->addr)
      return 0;
    else
      return 1;
  }
  static bool lt(const Node* a, const Node* b) noexcept {
    return a->addr < b->addr;
  }
};

struct DescriptionSzAdRbAccessor {
  using Node = Description;
  static constexpr auto link = &Node::link_szad;

  static int cmp(size_t size, const Node* b) noexcept {
    if (size <= b->size)
      return -1;
    else
      return 1;
  }
  static bool lt(const Node* a, const Node* b) noexcept {
    return (a->size < b->size) || (a->size == b->size && a->addr < b->addr);
  }
};

struct PageTree {
  Rb<DescriptionAdRbAccessor> ad;
  Rb<DescriptionSzAdRbAccessor> szad;
};

class PageTreeAllocator {
 public:
  constexpr PageTreeAllocator() noexcept = default;
  PageTreeAllocator(const PageTreeAllocator&) = delete;
  PageTreeAllocator& operator=(const PageTreeAllocator&) = delete;

  Page* allocate(size_t size) noexcept;
  bool reclaim(Page* page, size_t size, uint32_t option_bitmask) noexcept;
  bool reclaim_nomerge(Page* page, size_t size) noexcept;
  bool extend_nomove(Page* ptr, size_t old, size_t grow) noexcept;

  Description* get_deallocate_candidates(size_t threshold) noexcept;

 private:
  PageTree tree_;
};

Page* PageTreeAllocator::allocate(size_t size) noexcept {
  Description* desc = tree_.szad.nsearch(size);
  if (desc) {
    assert(desc->size >= size);
    Page* ret;
    desc = tree_.szad.remove(desc);
    if (desc->size == size) {
      // Perfect size.
      desc = tree_.ad.remove(desc);
      ret = desc->addr;
      free_description(desc);
    } else {
      // Use the latter portion, and return the rest
      ret = byte_advance(desc->addr, desc->size - size);
      desc->size -= size;
      desc = tree_.szad.insert(desc);
    }
    return ret;
  } else {
    return nullptr;
  }
}

bool PageTreeAllocator::reclaim(Page* page, size_t size,
                                uint32_t option_bitmask) noexcept {
  CBU_HINT_ASSERT(page != nullptr);
  assert(size != 0);
  assert(size % kPageSize == 0);

  Description* desc;

  // Can we merge right?
  Description* succ = (option_bitmask & RECLAIM_PAGE_NOMERGE_RIGHT)
                          ? nullptr
                          : tree_.ad.search(byte_advance(page, size));
  if (succ) succ = tree_.szad.remove(succ);

  // Can we merge left?
  Description* prec = (option_bitmask & RECLAIM_PAGE_NOMERGE_LEFT)
                          ? nullptr
                          : tree_.ad.psearch(page);
  if (prec && byte_advance(prec->addr, prec->size) != page) prec = nullptr;

  if (prec) {  // Can merge backward.
    prec = tree_.szad.remove(prec);
    if (succ) {  // Both!
      size += succ->size;
      succ = tree_.ad.remove(succ);
      free_description(succ);
      // prec's position in tree.ad needs no change.
    }
    prec->size += size;
    desc = prec;
  } else if (succ) {  // Forward only
    // succ's position in tree.ad needs no change.
    succ->addr = page;
    succ->size += size;
    desc = succ;
  } else {  // Neither.
    desc = alloc_description();
    if (false_no_fail(!desc)) return false;
    desc->addr = page;
    desc->size = size;
    desc = tree_.ad.insert(desc);
  }

  tree_.szad.insert(desc);
  return true;
}

bool PageTreeAllocator::reclaim_nomerge(Page* page, size_t size) noexcept {
  CBU_HINT_ASSERT(page != nullptr);
  assert(size != 0);
  assert(size % kPageSize == 0);

  Description* desc = alloc_description();
  if (false_no_fail(!desc)) return false;
  desc->addr = page;
  desc->size = size;
  desc = tree_.ad.insert(desc);
  tree_.szad.insert(desc);
  return true;
}

bool PageTreeAllocator::extend_nomove(Page* ptr, size_t old,
                                      size_t grow) noexcept {
  CBU_HINT_ASSERT(ptr != nullptr);
  CBU_HINT_ASSERT(old != 0);
  CBU_HINT_ASSERT(grow != 0);
  CBU_HINT_ASSERT(uintptr_t(ptr) % kPageSize == 0);
  CBU_HINT_ASSERT(old % kPageSize == 0);
  CBU_HINT_ASSERT(grow % kPageSize == 0);

  Page* target = byte_advance(ptr, old);

  Description* succ = tree_.ad.search(target);
  if (succ && (succ->size >= grow)) {
    // Yes
    succ = tree_.szad.remove(succ);
    if (succ->size == grow) {
      // Perfect size.
      succ = tree_.ad.remove(succ);
      free_description(succ);
      return true;
    } else {
      // Return the higher portion to tree
      succ->size -= grow;
      succ->addr = byte_advance(target, grow);
      succ = tree_.szad.insert(succ);
      return true;
    }
  } else {
    return false;
  }
}

Description* PageTreeAllocator::get_deallocate_candidates(
    size_t threshold) noexcept {
  Description* list = nullptr;

  Description* p = tree_.szad.last();

  while (p && (p->size >= threshold)) {
    Description* q = tree_.szad.prev(p);

    p = tree_.szad.remove(p);
    p = tree_.ad.remove(p);

    if (kTHPSize && kTHPSize * 2 < threshold) {
      // If we assume the kernel has support for transparent huge pages,
      // it's best if we always do allocations on hugepage boundaries

      // Split from right
      Page* end = byte_advance(p->addr, p->size);
      if (size_t offset = uintptr_t(end) % kTHPSize) {
        p->size -= offset;
        reclaim_nomerge(byte_back(end, offset), offset);
      }

      // Split from left
      if (size_t adjust = (-uintptr_t(p->addr)) % kTHPSize) {
        Page* addr = p->addr;
        p->addr = byte_advance(p->addr, adjust);
        p->size -= adjust;
        reclaim_nomerge(addr, adjust);
      }
    }

    p->link_ad.left(list);
    list = p;

    p = q;
  }

  return list;
}

struct alignas(kCacheLineSize) Arena {
  bool allow_thp;
  LowLevelMutex lock{};
  PageTreeAllocator tree{};

  static constexpr size_t kPreferredAllocSize =
      kTHPSize ? kTHPSize * 4 : kPageSize * 2048;
  static constexpr size_t kMunmapThreshold = 16 * 1024 * 1024;

  // Count it so that we can determine when to try to do munmap
  size_t reclaim_count = 0;

  // Use nomerge if we're sure it's not mergeable
  void reclaim_unlocked(Page* page, size_t size, uint32_t nomerge = 0) noexcept;

  Page* allocate(size_t size) noexcept;
  void reclaim(Page* page, size_t size, uint32_t nomerge = 0) noexcept;

  // Reclaim a list of pages, each of the same size.
  void reclaim_list(Page* page, size_t size) noexcept;

  bool extend_nomove(Page*, size_t oldsize, size_t growsize) noexcept;
  void do_munmap(Page*, size_t) noexcept;

  // Returns a linked list (linked with link_ad.left)
  Description* check_munmap_unlocked(
      size_t threshold = kMunmapThreshold) noexcept;
  void munmap_description_list(Description*) noexcept;
};

constinit Arena arena_default{true};
constinit Arena arena_no_thp{false};

Page* Arena::allocate(size_t size) noexcept {
  assert(size != 0);
  assert(size % kPageSize == 0);

  std::lock_guard locker(lock);

  Page* page = tree.allocate(size);
  if (!page) {
    // Must do actual allocation.
    size_t alloc_size = cbu::pow2_ceil(size, kPreferredAllocSize);
    page = (kTHPSize && !allow_thp ? RawPageAllocator::instance<false, Arena>
                                   : RawPageAllocator::instance<true, Arena>)
               .allocate(alloc_size);
    if (page == nullptr) return nomem();
    assert(size <= alloc_size);
    if (size < alloc_size)
      reclaim_unlocked(byte_advance(page, size), alloc_size - size,
                       RECLAIM_PAGE_NOMERGE_LEFT);
  }
  return page;
}

void Arena::reclaim_unlocked(Page* page, size_t size,
                             uint32_t nomerge) noexcept {
  reclaim_count += size;
  if (false_no_fail(!tree.reclaim(page, size, nomerge))) do_munmap(page, size);
}

void Arena::reclaim(Page* page, size_t size, uint32_t nomerge) noexcept {
  Description* clean;
  {
    std::lock_guard locker(lock);
    reclaim_unlocked(page, size, nomerge);
    // Check for whether we can do some clean up work.
    clean = check_munmap_unlocked();
  }
  munmap_description_list(clean);
}

void Arena::reclaim_list(Page* page, size_t size) noexcept {
  Description* clean;
  {
    std::lock_guard locker(lock);
    while (page) {
      Page* next = page->next;
      size_t this_size = size;

      // This should be pretty common in practice
      while (byte_advance(next, size) == page ||
             next == byte_advance(page, this_size)) {
        if (byte_advance(next, size) == page) page = next;
        this_size += size;
        next = next->next;
      }

      reclaim_unlocked(page, this_size);
      page = next;
    }
    clean = check_munmap_unlocked();
  }
  munmap_description_list(clean);
}

void Arena::do_munmap(Page* ptr, size_t size) noexcept {
  fsys_munmap(ptr, size);
}

bool Arena::extend_nomove(Page* ptr, size_t old, size_t grow) noexcept {
  std::lock_guard locker(lock);
  return tree.extend_nomove(ptr, old, grow);
}

Description* Arena::check_munmap_unlocked(size_t threshold) noexcept {
  if (reclaim_count < threshold * 2) return nullptr;
  reclaim_count = 0;
  return tree.get_deallocate_candidates(threshold);
}

void Arena::munmap_description_list(Description* clean) noexcept {
  while (clean) {
    Description* cur = clean;
    do_munmap(cur->addr, cur->size);
    clean = cur->link_ad.left();
    free_description(cur);
  }
}

#if defined __x86_64__ && defined __LP64__
constexpr size_t kPointerValidBits = 47;
#else
constexpr size_t kPointerValidBits = sizeof(void*) * 8;
#endif

// uint32_t is absolutely sufficient (4G * 4K is 16384 Gigabytes)
Trie<kPointerValidBits - kPageSizeBits, uint32_t> large_block_trie;

uint32_t* lookup_large_block(const Page* page) {
  return large_block_trie.lookup(reinterpret_cast<uintptr_t>(page) >>
                                 kPageSizeBits);
}

uint32_t* lookup_large_block_fail_crash(const Page* page) {
  return large_block_trie.lookup_fail_crash(reinterpret_cast<uintptr_t>(page) >>
                                            kPageSizeBits);
}

bool add_large_block_size(Page* page, size_t n) {
  uint32_t* ptr = lookup_large_block(page);
  if (false_no_fail(ptr == nullptr)) return false;
  std::atomic_ref(*ptr).store(n, std::memory_order_release);
  return true;
}

size_t lookup_large_block_size_fail_crash(Page* page) {
  uint32_t* ptr = lookup_large_block_fail_crash(page);
  return std::atomic_ref(*ptr).load(std::memory_order_acquire);
}

Page* allocate_page_uncached(size_t size, AllocateOptions options) {
  return (kTHPSize && !options.allow_thp ? arena_no_thp : arena_default)
      .allocate(size);
}

struct PageCategoryCache {
  // Cache of small pages
  static constexpr unsigned kPageMaxCategory = 7;
  static constexpr unsigned kPageCategories = kPageMaxCategory + 1;
  static constexpr unsigned kPagePreferredCount = 4;
  Page* page_list[kPageCategories] = {};
  unsigned char page_count[kPageCategories] = {};

  static constexpr unsigned size_to_page_category(size_t size) {
    return (size - kPageSize) / kPageSize;
  }
  static constexpr size_t page_category_to_size(unsigned cat) {
    return (unsigned(kPageSize) * (cat + 1));
  }

  void clear(Arena* arena) noexcept;
};

CachePool<PageCategoryCache> page_category_cache_pool;
CachePool<PageCategoryCache> page_category_cache_pool_no_thp;

}  // namespace

#if 0
void DescriptionCache::clear() noexcept {
  desc_count = 0;
  description_allocator.free_list(std::exchange(desc_list, nullptr));
}
#endif

void PageCategoryCache::clear(Arena* arena) noexcept {
  std::lock_guard locker_b(arena->lock);
  for (unsigned i = 0; i < kPageCategories; ++i) {
    Page* page = std::exchange(page_list[i], nullptr);
    page_count[i] = 0;
    while (page) {
      Page* next = page->next;
      arena->reclaim_unlocked(page, page_category_to_size(i));
      page = next;
    }
  }
}

void reclaim_page(Page* page, size_t size, uint32_t option_bitmask) noexcept {
  CBU_HINT_ASSERT(page != nullptr);
  assert(size != 0);
  assert(size % kPageSize == 0);

  bool use_no_thp = kTHPSize && (option_bitmask & RECLAIM_PAGE_NO_THP);

  Arena* arena = use_no_thp ? &arena_no_thp : &arena_default;

  if (size <= PageCategoryCache::page_category_to_size(
                  PageCategoryCache::kPageMaxCategory)) {
    UniqueCache unique_cache(use_no_thp ? &page_category_cache_pool_no_thp
                                        : &page_category_cache_pool);
    if (unique_cache) {
      PageCategoryCache& page_cache = *unique_cache;

      size_t cat = PageCategoryCache::size_to_page_category(size);

      Page* cache_head = page_cache.page_list[cat];
      page->next = cache_head;

      if (page_cache.page_count[cat] <
          PageCategoryCache::kPagePreferredCount * 2) {
        page_cache.page_count[cat]++;
        page_cache.page_list[cat] = page;
      } else {
        // Keep some, and free the rest
        page_cache.page_count[cat] -= PageCategoryCache::kPagePreferredCount;

        Page* check = cache_head;
        for (unsigned count = 1; count < PageCategoryCache::kPagePreferredCount;
             ++count)
          check = check->next;
        page_cache.page_list[cat] = std::exchange(check->next, nullptr);

        arena->reclaim_list(page, size);
      }
      return;
    }
  }

  arena->reclaim(page, size, option_bitmask);
}

void reclaim_page(Page* ptr, size_t size, AllocateOptions options) noexcept {
  reclaim_page(ptr, size, options.allow_thp ? 0 : RECLAIM_PAGE_NO_THP);
}

// Allocate contiguous pages of size bytes
Page* allocate_page(size_t size, AllocateOptions options) noexcept {
  assert(size != 0);
  assert(size % kPageSize == 0);

  bool use_no_thp = kTHPSize && !options.allow_thp;

  if (size <= PageCategoryCache::page_category_to_size(
                  PageCategoryCache::kPageMaxCategory)) {
    UniqueCache unique_cache(use_no_thp ? &page_category_cache_pool_no_thp
                                        : &page_category_cache_pool);
    if (unique_cache) {
      PageCategoryCache& page_cache = *unique_cache;

      size_t cat = PageCategoryCache::size_to_page_category(size);
      if (page_cache.page_list[cat]) {
        Page* ret = page_cache.page_list[cat];
        page_cache.page_list[cat] = ret->next;
        page_cache.page_count[cat]--;
        return ret;
      }

      // Let's allocate 2 and cache the other
      const size_t alloc_size = size * 2;
      Page* page = allocate_page_uncached(alloc_size, options);
      if (false_no_fail(page == nullptr)) return nullptr;
      Page* rpage = byte_advance(page, size);
      page_cache.page_count[cat] = 1;
      rpage->next = nullptr;  // page_cache.page_list[cat];
      page_cache.page_list[cat] = rpage;
      return page;
    }
  }

  return allocate_page_uncached(size, options);
}

void* alloc_large(size_t n) noexcept {
  n = pagesize_ceil(n);
  if constexpr (sizeof(void*) > 4) {
    // We don't support allocating more than 4 gigabytes at one time
    if (n != uint32_t(n)) return nomem();
    n = uint32_t(n);
  }
  Page* page = allocate_page(n);
  if (false_no_fail(!page)) return nullptr;
  if (!add_large_block_size(page, n)) {
    reclaim_page(page, n);
    return nullptr;
  }
  return page;
}

void free_large(void* ptr) noexcept {
  Page* page = static_cast<Page*>(ptr);
  size_t size = lookup_large_block_size_fail_crash(page);
  reclaim_page(page, size);
}

void free_large(void* ptr, size_t size) noexcept {
  Page* page = static_cast<Page*>(ptr);
  size = pagesize_ceil(size);
  reclaim_page(page, size);
}

void* realloc_large(void* ptr, size_t newsize) noexcept {
  // Caller guarantees newsize is not 0
  newsize = pagesize_ceil(newsize);
  Page* page = (Page*)ptr;
  uint32_t* desc = lookup_large_block_fail_crash((Page*)ptr);
  size_t oldsize = std::atomic_ref(*desc).load(std::memory_order_acquire);
  if (oldsize == newsize) {
    return ptr;
  } else if (oldsize > newsize) {
    // Shrink
    std::atomic_ref(*desc).store(newsize, std::memory_order_release);
    reclaim_page(byte_advance(page, newsize), oldsize - newsize,
                 RECLAIM_PAGE_NOMERGE_LEFT);
    return ptr;
  } else {
    // Extend
    Arena* arena = &arena_default;
    if (arena->extend_nomove((Page*)ptr, oldsize, newsize - oldsize)) {
      std::atomic_ref(*desc).store(newsize, std::memory_order_release);
      return ptr;
    } else {
      void* nptr = alloc_large(newsize);
      if (true_no_fail(nptr)) {
        // std::atomic_ref(*desc).store(0, std::memory_order_release);
        nptr = memcpy(nptr, ptr, oldsize);
        reclaim_page(page, oldsize);
      }
      return nptr;
    }
  }
}

size_t large_allocated_size(const void* ptr) noexcept {
  return *lookup_large_block((const Page*)ptr);
}

void large_trim(size_t pad) noexcept {
  UniqueCache<PageCategoryCache>::visit_all(
      &page_category_cache_pool,
      [](PageCategoryCache* tc) noexcept { tc->clear(&arena_default); });
  if constexpr (kTHPSize > 0)
    UniqueCache<PageCategoryCache>::visit_all(
        &page_category_cache_pool_no_thp,
        [](PageCategoryCache* tc) noexcept { tc->clear(&arena_no_thp); });

  {
    Arena* arena = &arena_default;
    Description* clean_list =
        (std::lock_guard(arena->lock), arena->check_munmap_unlocked(pad));
    arena->munmap_description_list(clean_list);
  }

  if constexpr (kTHPSize > 0) {
    Arena* arena = &arena_no_thp;
    Description* clean_list =
        (std::lock_guard(arena->lock), arena->check_munmap_unlocked(pad));
    arena->munmap_description_list(clean_list);
  }

  // Doesn't make much sense to free description_cache - that's allocated from
  // PermaAlloc and never returned to system
}

}  // namespace alloc
}  // namespace cbu
// vim: fdm=marker:
