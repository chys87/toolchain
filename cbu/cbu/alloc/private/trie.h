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
#include <string.h>

#include <memory>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/private/common.h"
#include "cbu/alloc/private/permanent.h"
#include "cbu/common/bit.h"

namespace cbu {
namespace alloc {

template <typename Node>
Node* ensure_node_heavy(Node **ptr, SimplePermaAlloc<Node> &allocator) {
  Node* next = allocator.alloc();
  if (false_no_fail(next == NULL))
    return NULL;
  *next = {};
#ifdef CBU_SINGLE_THREADED
  *ptr = next;
#else
  Node* got = 0;
  if (!std::atomic_ref<Node*>(*ptr).compare_exchange_strong(
          got, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
    Node* rgot = got;
    allocator.free(next);
    next = rgot;
  }
#endif
  return next;
}

template <typename Node>
inline Node* ensure_node(Node** ptr, SimplePermaAlloc<Node>& allocator) {
  Node* next = load_acquire(ptr);
  if (next == nullptr) next = ensure_node_heavy(ptr, allocator);
  return next;
}

inline constexpr unsigned const_log2(size_t n) noexcept {
  unsigned i = 0;
  while (n > 1) {
    ++i;
    n /= 2;
  }
  return i;
}

template <unsigned TotalBits, typename ValueType = uintptr_t>
requires (TotalBits >= 8 and TotalBits <= sizeof(uintptr_t) * 8 and
          (sizeof(ValueType) & (sizeof(ValueType) - 1)) == 0)
class Trie {
 private:
  union Node;
  static constexpr unsigned NodeSize = 64;
  static constexpr unsigned LevelBits =
      const_log2(NodeSize / sizeof(uintptr_t));
  static constexpr unsigned LeafBits = const_log2(NodeSize / sizeof(ValueType));
  static constexpr unsigned Levels = (TotalBits - LeafBits) / LevelBits;
  static constexpr unsigned TopBits = (TotalBits - LeafBits) % LevelBits;

  union Node {
    alignas(NodeSize) Node* link[1 << LevelBits];
    alignas(NodeSize) ValueType tbl[1 << LeafBits];
  };

 public:
  ValueType* lookup_fail_crash(uintptr_t);
  ValueType* lookup(uintptr_t);

 private:
  SimplePermaAlloc<Node> allocator_ {};
  Node* head_[1 << TopBits] {};
};

template <unsigned TotalBits, typename ValueType>
requires (TotalBits >= 8 and TotalBits <= sizeof(uintptr_t) * 8 and
          (sizeof(ValueType) & (sizeof(ValueType) - 1)) == 0)
ValueType* Trie<TotalBits, ValueType>::lookup_fail_crash(uintptr_t v) {
  assert(v < ((uintptr_t(1) << TotalBits) - 1));
  uintptr_t levelmask = (uintptr_t(1) << LevelBits) - 1;
  Node* node = reinterpret_cast<Node *>(
      head_[TopBits ? (v >> (Levels * LevelBits + LeafBits)) : 0]);
#pragma GCC unroll 16
  for (int i = (Levels - 1) * LevelBits; i >= 0; i -= LevelBits) {
    size_t o = (v >> (i + LeafBits)) & levelmask;
    node = load_acquire(&node->link[o]);
  }
  return &node->tbl[v & ((1 << LeafBits) - 1)];
}

template <unsigned TotalBits, typename ValueType>
requires (TotalBits >= 8 and TotalBits <= sizeof(uintptr_t) * 8 and
          (sizeof(ValueType) & (sizeof(ValueType) - 1)) == 0)
ValueType* Trie<TotalBits, ValueType>::lookup(uintptr_t v) {
  assert(v < ((uintptr_t(1) << TotalBits) - 1));
  uintptr_t levelmask = (uintptr_t(1) << LevelBits) - 1;

  Node** pnode = &head_[TopBits ? (v >> (Levels * LevelBits + LeafBits)) : 0];
  Node* node = *pnode;
  for (int i = Levels * LevelBits; ;) {
    node = ensure_node(pnode, allocator_);
    i -= LevelBits;
    if (i < 0)
      break;
    pnode = &node->link[(v >> (i + LeafBits)) & levelmask];
  }
  return &node->tbl[v & ((1 << LeafBits) - 1)];
}

}  // namespace alloc
}  // namespace cbu
