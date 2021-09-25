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
#include <compare>
#include <mutex>
#include <optional>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/private.h"
#include "cbu/alloc/rb.h"
#include "cbu/alloc/tc.h"
#include "cbu/alloc/trie.h"
#include "cbu/common/byte_size.h"
#include "cbu/common/hint.h"
#include "cbu/fsyscall/fsyscall.h"
#include "cbu/tweak/tweak.h"

namespace cbu {
namespace alloc {
namespace {

union Description {
  struct {
    RbLink<Description> rblink_1;
    RbLink<Description> rblink_2;
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

template <int LinkIdx = 1>
struct DescriptionAdRbAccessor {
  using Node = Description;
  static constexpr auto link =
      (LinkIdx == 1) ? &Node::rblink_1 : &Node::rblink_2;

  static std::weak_ordering cmp(const Page* a, const Node* b) noexcept {
    return a <=> b->addr;
  }
  static bool lt(const Node* a, const Node* b) noexcept {
    return a->addr < b->addr;
  }
};

struct DescriptionSzAdRbAccessor {
  using Node = Description;
  static constexpr auto link = &Node::rblink_2;

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

class PageTreeAllocator {
 public:
  constexpr PageTreeAllocator() noexcept = default;
  PageTreeAllocator(const PageTreeAllocator&) = delete;
  PageTreeAllocator& operator=(const PageTreeAllocator&) = delete;

  Page* allocate(size_t size) noexcept;
  bool reclaim(Page* page, size_t size, uint32_t option_bitmask) noexcept;
  bool reclaim_nomerge(Page* page, size_t size) noexcept;
  bool extend_nomove(Page* ptr, size_t old, size_t grow) noexcept;

  // Returns a linked list of Description (linked with rblink_1.left),
  // so that the caller may decide to free it.
  Description* get_deallocate_candidates(size_t threshold,
                                         bool thp_aware) noexcept;

  // Just remove the page from the tree
  void remove_by_range(Page* page, size_t size) noexcept;
  // Likewise, but calls a callback on the removed subrange
  template <typename Callback>
  void remove_by_range(Page* page, size_t size, Callback callback) noexcept;
  // Just remove a list of ranges (linked with rblink_1.left).
  // The list itself is not removed.
  void remove_by_list(const Description* list) noexcept;

 private:
  Description* remove_from_szad(Description* desc) noexcept;
  Description* insert_to_szad(Description* desc) noexcept;

  static constexpr size_t small_size_to_idx(size_t size) noexcept {
    // This is written in a way that helps x86-64 generate best code
    return size_t(unsigned(size) / kPageSize) - 1;
  }
  static constexpr size_t small_idx_to_size(unsigned idx) noexcept {
    return (idx + 1) * unsigned(kPageSize);
  }

 private:
  Rb<DescriptionAdRbAccessor<1>> ad_;

  static constexpr size_t kSmallCount = 4;
  static constexpr size_t kSmallMaxSize = kSmallCount * kPageSize;
  Rb<DescriptionAdRbAccessor<2>> szad_small_[kSmallCount];
  Rb<DescriptionSzAdRbAccessor> szad_large_;
};

Description* PageTreeAllocator::remove_from_szad(Description* desc) noexcept {
  if (desc->size <= kSmallMaxSize)
    return szad_small_[small_size_to_idx(desc->size)].remove(desc);
  else
    return szad_large_.remove(desc);
}

Description* PageTreeAllocator::insert_to_szad(Description* desc) noexcept {
  if (desc->size <= kSmallMaxSize)
    return szad_small_[small_size_to_idx(desc->size)].insert(desc);
  else
    return szad_large_.insert(desc);
}

Page* PageTreeAllocator::allocate(size_t size) noexcept {
  if (size <= kSmallMaxSize) {
    size_t k = small_size_to_idx(size);
    if (Description* desc = szad_small_[k].first()) {
      desc = szad_small_[k].remove(desc);
      desc = ad_.remove(desc);
      Page* ret = desc->addr;
      free_description(desc);
      return ret;
    }

    while (++k < kSmallCount) {
      if (Description* desc = szad_small_[k].first()) {
        desc = szad_small_[k].remove(desc);
        Page* ret = desc->addr;
        desc->addr = byte_advance(ret, size);
        desc->size = small_idx_to_size(k) - size;
        szad_small_[small_size_to_idx(desc->size)].insert(desc);
        return ret;
      }
    }
  }

  Description* desc = szad_large_.nsearch(size);
  if (desc) {
    assert(desc->size >= size);
    Page* ret;
    desc = szad_large_.remove(desc);
    if (desc->size == size) {
      // Perfect size.
      desc = ad_.remove(desc);
      ret = desc->addr;
      free_description(desc);
    } else {
      // We always prefer smaller addresses
      ret = desc->addr;
      desc->addr = byte_advance(desc->addr, size);
      desc->size -= size;
      insert_to_szad(desc);
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
                          : ad_.search(byte_advance(page, size));
  if (succ) succ = remove_from_szad(succ);

  // Can we merge left?
  Description* prec = (option_bitmask & RECLAIM_PAGE_NOMERGE_LEFT)
                          ? nullptr
                          : ad_.psearch(page);
  if (prec && byte_advance(prec->addr, prec->size) != page) prec = nullptr;

  if (prec) {  // Can merge backward.
    prec = remove_from_szad(prec);
    if (succ) {  // Both!
      size += succ->size;
      succ = ad_.remove(succ);
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
    desc = ad_.insert(desc);
  }

  insert_to_szad(desc);
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
  desc = ad_.insert(desc);
  insert_to_szad(desc);
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

  Description* succ = ad_.search(target);
  if (succ && (succ->size >= grow)) {
    // Yes
    succ = remove_from_szad(succ);
    if (succ->size == grow) {
      // Perfect size.
      succ = ad_.remove(succ);
      free_description(succ);
      return true;
    } else {
      // Return the higher portion to tree
      succ->size -= grow;
      succ->addr = byte_advance(target, grow);
      succ = insert_to_szad(succ);
      return true;
    }
  } else {
    return false;
  }
}

Description* PageTreeAllocator::get_deallocate_candidates(
    size_t threshold, bool thp_aware) noexcept {
  Description* list = nullptr;

  Description* p = szad_large_.last();

  while (p && (p->size >= threshold)) {
    Description* q = szad_large_.prev(p);

    p = szad_large_.remove(p);
    p = ad_.remove(p);

    if constexpr (kTHPSize) {
      if (thp_aware && kTHPSize * 2 < threshold) {
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
    }

    p->rblink_1.left(list);
    list = p;

    p = q;
  }

  for (size_t i = kSmallMaxSize; i > 0 && i >= threshold; i -= kPageSize) {
    size_t idx = small_size_to_idx(i);
    auto& tree = szad_small_[idx];
    while (Description* p = tree.first()) {
      p = tree.remove(p);
      p = ad_.remove(p);
      p->rblink_1.left(list);
      list = p;
    }
  }

  return list;
}

void PageTreeAllocator::remove_by_range(Page* page, size_t size) noexcept {
  remove_by_range(page, size, [](void*, size_t) noexcept {});
}

template <typename Callback>
void PageTreeAllocator::remove_by_range(Page* page, size_t size,
                                        Callback callback) noexcept {
  Page* end = byte_advance(page, size);

  Description* p = ad_.psearch(page);

  if (p) {
    // The first node should be handled differently
    if (page <= p->addr) {
      // Nothing to do.  Fall through to the regular logic.
    } else {
      Page* p_end = byte_advance(p->addr, p->size);
      if (page < p_end) {
        if (end < p_end) {
          // Special case - the range is in the middle of p_prev
          callback(page, size);
          p = remove_from_szad(p);
          p->size = byte_distance(p->addr, page);
          insert_to_szad(p);

          // There is nothing we can really do if alloc_description fails.
          // So we simply ignore the error.
          Description* p_new = alloc_description();
          if (true_no_fail(p_new)) {
            p_new->addr = end;
            p_new->size = byte_distance(end, p_end);
            p_new = ad_.insert(p_new);
            p_new = insert_to_szad(p_new);
          }
          return;

        } else {
          // The range covers a suffix of the node
          callback(page, byte_distance(page, p_end));
          p = remove_from_szad(p);
          p->size = byte_distance(p->addr, page);
          p = insert_to_szad(p);
          if (end == p_end) return;
        }
      }
      p = ad_.next(p);
    }
  } else {
    p = ad_.first();
  }

  while (p) {
    // Handle and skip the gap
    if (end <= p->addr) return;
    page = p->addr;

    Page* p_end = byte_advance(p->addr, p->size);

    // Now we have guarantee that page == p->addr
    if (end < p_end) {
      // The range is prefix of the node -- modify node and return
      p = remove_from_szad(p);
      p->addr = end;
      p->size = byte_distance(end, p_end);
      p = insert_to_szad(p);
      callback(page, byte_distance(page, end));
      return;
    } else {
      // Otheriwse, we should remove the whole node
      callback(page, p->size);
      auto next_p = ad_.next(p);
      p = ad_.remove(p);
      p = remove_from_szad(p);
      free_description(p);
      p = next_p;
      page = p_end;
    }
  }
}

void PageTreeAllocator::remove_by_list(const Description* list) noexcept {
  while (list) {
    remove_by_range(list->addr, list->size);
    list = list->rblink_1.left();
  }
}

class alignas(kCacheLineSize) Arena {
 public:
  constexpr Arena(RawPageAllocator* allocator) noexcept
      : raw_page_allocator_(allocator) {}

  // Set option_bitmask with NOMERGE_LEFT/NOMERGE_RIGHT if we're sure it's not
  // mergeable.
  // Set RECLAIM_PAGE_CLEAN if the caller can guarantee the page is clean
  void reclaim_unlocked(Page* page, size_t size,
                        uint32_t option_bitmask = 0) noexcept;

  Page* allocate(size_t size, bool zero) noexcept;
  void reclaim(Page* page, size_t size, uint32_t option_bitmask = 0) noexcept;

  // Reclaim a list of pages, each of the same size.
  void reclaim_list(Page* page, size_t size) noexcept;

  bool extend_nomove(Page*, size_t oldsize, size_t growsize) noexcept;
  void discard(Page*, size_t) noexcept;

  // Returns a linked list (linked with rblink_1.left)
  Description* trim_and_extract_unlocked(
      std::optional<size_t> threshold = std::nullopt) noexcept;

  Description* trim_and_extract(
      std::optional<size_t> threshold = std::nullopt) noexcept;

  void clear_description_list(Description*) noexcept;

  friend struct PageCategoryCache;

 private:
  RawPageAllocator* const raw_page_allocator_;
  LowLevelMutex lock_{};

  // Count it so that we can determine when to try to do munmap
  size_t reclaim_count_ = 0;
  size_t total_bytes_allocated_ = 0;

  // tree_clean holds pages that we know are zero initialized
  PageTreeAllocator tree_clean_{};
  PageTreeAllocator tree_dirty_{};
  PageTreeAllocator tree_all_{};

  static constexpr size_t kInitialAllocSize =
      kTHPSize ? kTHPSize : kPageSize * 512;
  static constexpr size_t kMaxAllocSize = 128 * 1024 * 1024;
  static_assert((kInitialAllocSize & (kInitialAllocSize - 1)) == 0);
};

// No additional tag types are given to RawPageAllocator::instance, meaning the
// "default" allocator.
constinit Arena arena_brk{&RawPageAllocator::instance_brk};
constinit Arena arena_mmap{&RawPageAllocator::instance_mmap};
constinit Arena arena_mmap_no_thp{&RawPageAllocator::instance_mmap_no_thp};

Page* Arena::allocate(size_t size, bool zero) noexcept {
  assert(size != 0);
  assert(size % kPageSize == 0);

  std::lock_guard locker(lock_);

  // First try allocating from one of tree_clean_ and tree_dirty_,
  Page* page = (zero ? tree_clean_ : tree_dirty_).allocate(size);

  if (page) {
    // Remove pages from tree_all as well
    tree_all_.remove_by_range(page, size);
  } else {
    // If allocation from tree_clean/tree_dirty fails, try allocating from
    // tree_all
    page = tree_all_.allocate(size);
    if (page) {
      tree_clean_.remove_by_range(page, size);
      if (zero) {
        tree_dirty_.remove_by_range(page, size,
                                    [](Page* subrange, size_t subsize) {
                                      memset(subrange, 0, subsize);
                                    });
      } else {
        tree_dirty_.remove_by_range(page, size);
      }
    }
  }

  if (!page) {
    size_t alloc_size = cbu::pow2_ceil(
        std::max(std::min(total_bytes_allocated_, kMaxAllocSize), size),
        kInitialAllocSize);
    page = raw_page_allocator_->allocate(alloc_size);
    if (page == nullptr) return nullptr;
    if (size < alloc_size)
      reclaim_unlocked(byte_advance(page, size), alloc_size - size,
                       RECLAIM_PAGE_NOMERGE_LEFT | RECLAIM_PAGE_CLEAN);
  }
  total_bytes_allocated_ += size;
  return page;
}

void Arena::reclaim_unlocked(Page* page, size_t size,
                             uint32_t option_bitmask) noexcept {
  // Don't modify total_bytes_allocated_ here -- this function is also
  // called from allocate.
  if (!(option_bitmask & RECLAIM_PAGE_CLEAN)) reclaim_count_ += size;

  if (false_no_fail(!tree_all_.reclaim(page, size, option_bitmask))) {
    discard(page, size);
    return;
  }

  auto& tree =
      (option_bitmask & RECLAIM_PAGE_CLEAN) ? tree_clean_ : tree_dirty_;
  if (false_no_fail(!tree.reclaim(page, size, option_bitmask))) {
    tree_all_.remove_by_range(page, size);
    discard(page, size);
  }
}

void Arena::reclaim(Page* page, size_t size, uint32_t option_bitmask) noexcept {
  Description* clean;
  {
    std::lock_guard locker(lock_);
    total_bytes_allocated_ -= size;
    reclaim_unlocked(page, size, option_bitmask);
    // Check for whether we can do some clean up work.
    clean = trim_and_extract_unlocked();
  }
  clear_description_list(clean);
}

void Arena::reclaim_list(Page* page, size_t size) noexcept {
  Description* clean;
  {
    std::lock_guard locker(lock_);
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

      total_bytes_allocated_ -= this_size;
      reclaim_unlocked(page, this_size);
      page = next;
    }
    clean = trim_and_extract_unlocked();
  }
  clear_description_list(clean);
}

void Arena::discard(Page* ptr, size_t size) noexcept {
  if (raw_page_allocator_->use_brk())
    fsys_madvise(ptr, size, MADV_DONTNEED);
  else
    fsys_munmap(ptr, size);
}

bool Arena::extend_nomove(Page* ptr, size_t old, size_t grow) noexcept {
  std::lock_guard locker(lock_);
  // Try tree_clean first, which is more likely to succeed
  if (!tree_clean_.extend_nomove(ptr, old, grow) &&
               !tree_dirty_.extend_nomove(ptr, old, grow))
    return false;
  tree_all_.remove_by_range(byte_advance(ptr, old), grow);
  return true;
}

Description* Arena::trim_and_extract_unlocked(
    std::optional<size_t> threshold_opt) noexcept {
  size_t threshold = threshold_opt
                         ? *threshold_opt
                         : std::max(total_bytes_allocated_, kInitialAllocSize);

  if (reclaim_count_ < threshold * 2) return nullptr;
  reclaim_count_ = 0;
  // We only check tree_dirty.  This is probably OK.
  // Pages in tree_clean_ are most likely not populated by kernel yet.
  Description* list = tree_dirty_.get_deallocate_candidates(
      threshold, kTHPSize && raw_page_allocator_->allow_thp());
  // Even if we use BRK, we still have to remove the memory out of tree_all_,
  // and then add them back, because we will run madvise without holding the
  // lock.
  tree_all_.remove_by_list(list);
  return list;
}

Description* Arena::trim_and_extract(std::optional<size_t> threshold) noexcept {
  std::lock_guard locker(lock_);
  return trim_and_extract_unlocked(threshold);
}

void Arena::clear_description_list(Description* clean) noexcept {
  if (raw_page_allocator_->use_brk()) {
    for (Description* cur = clean; cur; cur = cur->rblink_1.left())
      fsys_madvise(cur->addr, cur->size, MADV_DONTNEED);
    std::lock_guard locker(lock_);
    while (clean) {
      Description* cur = clean;
      reclaim_unlocked(cur->addr, cur->size, RECLAIM_PAGE_CLEAN);
      clean = cur->rblink_1.left();
      free_description(cur);
    }
  } else {
    while (clean) {
      Description* cur = clean;
      fsys_munmap(cur->addr, cur->size);
      clean = cur->rblink_1.left();
      free_description(cur);
    }
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
  if (tweak::USE_BRK && !options.force_mmap) {
    Page* page = arena_brk.allocate(size, options.zero);
    if (page) return page;
  }
  return (kTHPSize && options.force_mmap ? arena_mmap_no_thp : arena_mmap)
      .allocate(size, options.zero);
}

struct PageCategoryCache {
  // Cache of small pages
  static constexpr unsigned kPageMaxCategory = 7;
  static constexpr unsigned kPageCategories = kPageMaxCategory + 1;
  static constexpr unsigned kPagePreferredCount = 4;
  Page* page_list[kPageCategories] = {};
  unsigned char page_count[kPageCategories] = {};

  static constexpr unsigned size_to_page_category(size_t size) {
    return unsigned(size - kPageSize) / kPageSize;
  }
  static constexpr size_t page_category_to_size(unsigned cat) {
    return (unsigned(kPageSize) * (cat + 1));
  }

  void clear(Arena* arena) noexcept;
};

void PageCategoryCache::clear(Arena* arena) noexcept {
  std::lock_guard locker_b(arena->lock_);
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

CachePool<PageCategoryCache> page_category_cache_pool_brk;
CachePool<PageCategoryCache> page_category_cache_pool;
CachePool<PageCategoryCache> page_category_cache_pool_no_thp;

Page* try_allocate_from_cache_pool(CachePool<PageCategoryCache>* pool,
                                   size_t cat) {
  UniqueCache unique_cache(pool);
  if (unique_cache) {
    PageCategoryCache& page_cache = *unique_cache;
    if (page_cache.page_list[cat]) {
      Page* ret = page_cache.page_list[cat];
      page_cache.page_list[cat] = ret->next;
      page_cache.page_count[cat]--;
      return CBU_HINT_NONNULL(ret);
    }
  }
  return nullptr;
}

}  // namespace

#if 0
void DescriptionCache::clear() noexcept {
  desc_count = 0;
  description_allocator.free_list(std::exchange(desc_list, nullptr));
}
#endif

void reclaim_page(Page* page, size_t size, uint32_t option_bitmask) noexcept {
  CBU_HINT_ASSERT(page != nullptr);
  assert(size != 0);
  assert(size % kPageSize == 0);

  bool from_brk = RawPageAllocator::is_from_brk(page);
  bool use_no_thp = kTHPSize && (option_bitmask & RECLAIM_PAGE_NO_THP);

  Arena* arena = use_no_thp ? &arena_mmap_no_thp : &arena_mmap;

  if (size <= PageCategoryCache::page_category_to_size(
                  PageCategoryCache::kPageMaxCategory)) {
    UniqueCache unique_cache(from_brk     ? &page_category_cache_pool_brk
                             : use_no_thp ? &page_category_cache_pool_no_thp
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
  reclaim_page(ptr, size, options.force_mmap ? RECLAIM_PAGE_NO_THP : 0);
}

// Allocate contiguous pages of size bytes
Page* allocate_page(size_t size, AllocateOptions options) noexcept {
  assert(size != 0);
  assert(size % kPageSize == 0);

  if (!options.zero && size <= PageCategoryCache::page_category_to_size(
                                   PageCategoryCache::kPageMaxCategory)) {
    size_t cat = PageCategoryCache::size_to_page_category(size);
    if (tweak::USE_BRK && !options.force_mmap) {
      Page* ret =
          try_allocate_from_cache_pool(&page_category_cache_pool_brk, cat);
      if (ret) return ret;
    }

    auto* pool = (kTHPSize && options.force_mmap)
                     ? &page_category_cache_pool_no_thp
                     : &page_category_cache_pool;
    Page* ret = try_allocate_from_cache_pool(pool, cat);
    if (ret) return ret;
  }
  return allocate_page_uncached(size, options);
}

void* alloc_large(size_t n, bool zero) noexcept {
  n = pagesize_ceil(n);
  if constexpr (sizeof(void*) > 4) {
    // We don't support allocating more than 4 gigabytes at one time
    if (n != uint32_t(n)) return nomem();
    n = uint32_t(n);
  }
  Page* page = allocate_page(n, alloc::AllocateOptions().with_zero(zero));
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
    Arena* arena = &arena_mmap;
    if (arena->extend_nomove((Page*)ptr, oldsize, newsize - oldsize)) {
      std::atomic_ref(*desc).store(newsize, std::memory_order_release);
      return ptr;
    } else {
      void* nptr = alloc_large(newsize, false);
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
  if (tweak::USE_BRK) {
    UniqueCache<PageCategoryCache>::visit_all(
        &page_category_cache_pool_brk,
        [](PageCategoryCache* tc) noexcept { tc->clear(&arena_brk); });
  }
  UniqueCache<PageCategoryCache>::visit_all(
      &page_category_cache_pool,
      [](PageCategoryCache* tc) noexcept { tc->clear(&arena_mmap); });
  if constexpr (kTHPSize > 0)
    UniqueCache<PageCategoryCache>::visit_all(
        &page_category_cache_pool_no_thp,
        [](PageCategoryCache* tc) noexcept { tc->clear(&arena_mmap_no_thp); });

  if (tweak::USE_BRK) {
    Arena* arena = &arena_brk;
    Description* clean_list = arena->trim_and_extract(pad);
    arena->clear_description_list(clean_list);
  }

  {
    Arena* arena = &arena_mmap;
    Description* clean_list = arena->trim_and_extract(pad);
    arena->clear_description_list(clean_list);
  }

  if constexpr (kTHPSize > 0) {
    Arena* arena = &arena_mmap_no_thp;
    Description* clean_list = arena->trim_and_extract(pad);
    arena->clear_description_list(clean_list);
  }

  // Doesn't make much sense to free description_cache - that's allocated from
  // PermaAlloc and never returned to system
}

}  // namespace alloc
}  // namespace cbu
// vim: fdm=marker:
