/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cbu/common/arena.h"

#include <algorithm>
#include <new>

#include "cbu/alloc/pagesize.h"
#include "cbu/common/bit.h"

namespace cbu {

namespace {

// If memory allocation is assumed never to fail, valid arguments and
// successful allocation are preconditions instead of runtime-checked
// properties.  Violating either is undefined behavior.
constexpr bool kCheckArenaArguments = !cbu::kMemoryNoExcept;

template <typename Node>
std::pair<void*, Node*> create_raw_with_node(
    Arena& arena, std::size_t size,
    std::size_t align) noexcept(cbu::kMemoryNoExcept) {
  // The object ends at the node, so the deleter can derive its address by
  // subtracting its size from the node address.  If alignof(T) is smaller than
  // alignof(Node), put the necessary padding before the object (rather than
  // between the object and the node).  This keeps both the object and the node
  // suitably aligned without storing the object address in the node.
  align = std::max(align, alignof(Node));
  if constexpr (kCheckArenaArguments) {
    if (size > std::size_t(-1) - (align - 1) ||
        size > std::size_t(-1) - sizeof(Node)) {
      throw std::bad_alloc();
    }
  }
  std::size_t end = cbu::pow2_ceil(size, align);
  if constexpr (kCheckArenaArguments) {
    if (end > std::size_t(-1) - sizeof(Node)) throw std::bad_alloc();
  }
  std::byte* memory =
      static_cast<std::byte*>(arena.CreateRaw(end + sizeof(Node), align));
  std::byte* object = memory + (end - size);
  return {object, reinterpret_cast<Node*>(object + size)};
}

}  // namespace

Arena::~Arena() noexcept {
  DestructSingle(active_);
  DestructList(list_);
}

union Arena::Node {
  char mem[1];
  struct {
    union {
      Node* next;          // In list
      std::size_t offset;  // Active
    };
    std::size_t size;
  };

  void operator delete(Node* p, std::destroying_delete_t) noexcept {
    ::operator delete(p, p->size, std::align_val_t(kArenaMaxAlign));
  }
};

void* Arena::CreateRaw(std::size_t size,
                       std::size_t align) CBU_MEMORY_NOEXCEPT {
  if (size > (kMaxAlloc - sizeof(Node)) / 2) {
    // size is large, use dedicated node.
    // Put dedicated node to list_ (instead of active_) so as not to interfere
    // with regular (mostly small-sized) allocation.
    std::size_t header_size = cbu::pow2_ceil(sizeof(Node), align);
    if constexpr (kCheckArenaArguments) {
      if (size > std::size_t(-1) - header_size) throw std::bad_alloc();
    }
    std::size_t total_size = size + header_size;
    Node* node = static_cast<Node*>(
        ::operator new(total_size, std::align_val_t(kArenaMaxAlign)));
    node->size = total_size;
    node->next = std::exchange(list_, node);
    return node->mem + header_size;
  }

  // Check whether the current node suffices.
  if (Node* first = active_) {
    std::size_t offset = cbu::pow2_ceil(first->offset, align);
    if (offset + size <= first->size) {
      first->offset = offset + size;
      return first->mem + offset;
    }

    active_ = nullptr;
    first->next = std::exchange(list_, first);
  }

  // Create a new node.
  std::size_t header_size = cbu::pow2_ceil(sizeof(Node), align);
  std::size_t min_alloc_size = header_size + size;
  std::size_t preferred_size =
      list_ ? std::min(kMaxAlloc, list_->size * 2) : kInitialAlloc;
  std::size_t real_size = std::max(min_alloc_size, preferred_size);
  if (real_size > cbu::alloc::kPageSize / 4) {
    if constexpr (kCheckArenaArguments) {
      if (real_size > std::size_t(-1) / 2) throw std::bad_alloc();
    }
    real_size = cbu::pow2_ceil(real_size, cbu::alloc::kPageSize);
  } else {
    real_size = cbu::pow2_ceil_nonzero(uint32_t(real_size));
  }
  Node* node = static_cast<Node*>(
      ::operator new(real_size, std::align_val_t(kArenaMaxAlign)));
  node->size = real_size;
  node->offset = header_size + size;
  active_ = node;
  return node->mem + header_size;
}

std::pair<void*, arena_detail::ArenaDeleterNode*> Arena::CreateRawWithNode(
    std::size_t size, std::size_t align) CBU_MEMORY_NOEXCEPT {
  return create_raw_with_node<arena_detail::ArenaDeleterNode>(*this, size,
                                                              align);
}

std::pair<void*, arena_detail::ArenaDeleterArrayNode*>
Arena::CreateRawWithArrayNode(std::size_t size,
                              std::size_t align) CBU_MEMORY_NOEXCEPT {
  return create_raw_with_node<arena_detail::ArenaDeleterArrayNode>(*this, size,
                                                                   align);
}

void Arena::Return(void* ptr, std::size_t n) noexcept {
  if (active_ && active_->mem + active_->offset == static_cast<char*>(ptr) + n)
    active_->offset -= n;
}

const char* Arena::PushStringImpl(const void* data,
                                  size_t size) CBU_MEMORY_NOEXCEPT {
  char* p = Create<char[]>(size);
  __builtin_memcpy(p, data, size);
  return p;
}

const char* Arena::PushZStringImpl(const void* data,
                                   size_t size) CBU_MEMORY_NOEXCEPT {
  char* p = Create<char[]>(size + 1);
  __builtin_memcpy(p, data, size);
  p[size] = '\0';
  return p;
}

void Arena::DestructSingle(Node* node) noexcept { delete node; }

void Arena::DestructList(Node* first) noexcept {
  while (first) {
    Node* next = first->next;
    delete first;
    first = next;
  }
}

}  // namespace cbu
