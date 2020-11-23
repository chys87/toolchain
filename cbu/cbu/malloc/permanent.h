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

#include "cbu/fsyscall/fsyscall.h"
#include "cbu/malloc/private.h"
#include "cbu/sys/low_level_mutex.h"
#include <algorithm>
#include <mutex>

namespace cbu {
inline namespace cbu_malloc {

// size must be non-0, and multiple of pagesize
void* get_pages_for_perma(size_t size) noexcept;

// Type T must have next (T *) and count (unsigned) members
template <typename T>
requires (sizeof(T) <= pagesize and alignof(T) <= pagesize)
class PermaAlloc {
public:
  T* alloc ();
  void free(T* ptr) noexcept;

  // Always return exactly preferred_count nodes
  T* alloc_list(unsigned preferred_count);
  void free_list(T* ptr) noexcept;

private:
  LowLevelMutex lock_;
  T* list_ = nullptr;
};

template <typename T>
requires (sizeof(T) <= pagesize and alignof(T) <= pagesize)
T* PermaAlloc<T>::alloc() {
  if (std::lock_guard locker(lock_); list_) {
    T* node = list_;
    unsigned old_count = node->count;
    auto next = node->next;
    if (old_count > 1) {
      T* rnode = node + 1;
      list_ = rnode;
      rnode->next = next;
      rnode->count = old_count - 1;
    } else {
      list_ = next;
    }
    return node;
  }

  // Nothing available. Allocate new
  constexpr size_t alloc_size = pagesize_ceil(4 * sizeof(T));
  void* np = get_pages_for_perma(alloc_size);
  if (false_no_fail(np == NULL))
    return NULL;

  T* node = static_cast<T *>(np);
  T* rnode = node + 1;

  // Put rnode in list
  std::lock_guard locker(lock_);
  rnode->next = list_;
  rnode->count = alloc_size / sizeof(T) - 1;
  list_ = rnode;
  return node;
}

template <typename T>
requires (sizeof(T) <= pagesize and alignof(T) <= pagesize)
T *PermaAlloc<T>::alloc_list(unsigned preferred_count) {
  unsigned count = 0;
  using Ptr = decltype(T::next);
  Ptr ret;
  Ptr* tail = &ret;

  for (std::lock_guard locker(lock_); list_ && (count < preferred_count); ) {
    T* node = list_;
    *tail = node;
    unsigned old_count = node->count;
    auto next = node->next;
    if (count + old_count > preferred_count) {
      unsigned pick = preferred_count - count;
      list_ = node + pick;
      list_->next = next;
      list_->count = old_count - pick;
      node->next = nullptr;
      node->count = pick;
      return ret;
    } else {
      count += old_count;
      list_ = next;
      tail = &node->next;
    }
  }

  if (count < preferred_count) {
    // Insufficient. Allocate new.

    unsigned need_count = preferred_count - count;
    const size_t alloc_size = pagesize_ceil(need_count * sizeof(T));
    void* np = get_pages_for_perma(alloc_size);
    if (false_no_fail(np == nullptr)) {
      if (ret) {
        std::lock_guard locker(lock_);
        *tail = list_;
        list_ = ret;
      }
      return nullptr;
    }
    T* node = static_cast<T*>(np);
    unsigned allocated_count = alloc_size / sizeof(T);
    *tail = node;
    node->count = need_count;
    tail = &node->next;
    if (allocated_count > need_count) {
      T *rnode = node + need_count;
      rnode->count = allocated_count - need_count;
      std::lock_guard locker(lock_);
      rnode->next = list_;
      list_ = rnode;
    }
  }

  *tail = nullptr;
  return ret;
}

template <typename T>
requires (sizeof(T) <= pagesize and alignof(T) <= pagesize)
void PermaAlloc<T>::free(T* ptr) noexcept {
  std::lock_guard locker(lock_);
  ptr->next = list_;
  ptr->count = 1;
  list_ = ptr;
}

template <typename T>
requires (sizeof(T) <= pagesize and alignof(T) <= pagesize)
void PermaAlloc<T>::free_list(T* ptr) noexcept {
  if (!ptr)
    return;
  T* tail = ptr;
  while (tail->next)
    tail = tail->next;
  std::lock_guard locker(lock_);
  tail->next = list_;
  list_ = ptr;
}

template <size_t Size, size_t Align>
requires (Size <= pagesize)
class SizedPermaAlloc
{
  union Node {
    alignas(Align) char memory[Size];
    struct {
      Node *next;
      unsigned count;
    };
  };

  PermaAlloc<Node> allocator_;

public:
  inline void* alloc () { return allocator_.alloc(); }
  inline void free(void* ptr) { allocator_.free(static_cast<Node*>(ptr)); }
};

template <typename T>
class SimplePermaAlloc : public SizedPermaAlloc<sizeof(T), alignof(T)> {
private:
  using Base = SizedPermaAlloc<sizeof(T), alignof(T)>;

public:
  inline T* alloc() { return static_cast<T*>(Base::alloc()); }
  inline void free(T* ptr) { Base::free(ptr); }
};

} // namespace cbu_malloc
} // namespace cbu
