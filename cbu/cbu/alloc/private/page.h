/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include "cbu/alloc/private/common.h"
#include "cbu/alloc/private/rb.h"

namespace cbu::alloc {

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

template <RbLink<Description> Description::*rblink>
struct DescriptionAdRbAccessor {
  using Node = Description;
  static constexpr auto link = rblink;

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

  static constexpr unsigned small_size_to_idx(size_t size) noexcept {
    return (size >> kPageSizeBits) - 1;
  }
  static constexpr size_t small_idx_to_size(unsigned idx) noexcept {
    return (idx + 1) << kPageSizeBits;
  }

 private:
  Rb<DescriptionAdRbAccessor<&Description::rblink_1>> ad_;

  static constexpr size_t kSmallCount = 4;
  static constexpr size_t kSmallMaxSize = kSmallCount * kPageSize;
  Rb<DescriptionAdRbAccessor<&Description::rblink_2>> szad_small_[kSmallCount];
  Rb<DescriptionSzAdRbAccessor> szad_large_;
};

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
  [[no_unique_address]] LowLevelMutex lock_{};

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

  // Make sure kMinTrimThreshold is > 2 * kTHPSize so that we always do
  // trimming on THP boundary
  static constexpr size_t kMinTrimThreshold =
      kTHPSize ? 3 * kTHPSize : kPageSize * 1536;
  static constexpr size_t kMaxTrimThreshold = 128 * 1024 * 1024;
  static_assert(kMinTrimThreshold < kMaxTrimThreshold);
};

struct DescriptionCache {
  // Description cache
  Description* desc_list = nullptr;
  unsigned desc_count = 0;
  static constexpr size_t kPreferredCount = 16;

  void clear() noexcept;
};

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

}//
