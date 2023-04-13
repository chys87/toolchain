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

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <type_traits>

#include "cbu/common/byte_size.h"

// Heap operations.
//  heap_* are basic heap functions.
//  heapq_* are building blocks for an intrusive heap container (not included in
//                                                               cbu).
//
// Note that comparators are in the opposite way as the std heap functions.

namespace cbu {
namespace heapq_detail {

// Can be used to store the index into the positioned object
struct HeapDefaultPositioner {
  template <typename T>
  void operator()(T &, std::size_t) const {
  }
};

template <typename T, typename P>
inline constexpr void swap_pos(T* heap,
                               cbu::ByteSize<std::type_identity_t<T>> i,
                               cbu::ByteSize<std::type_identity_t<T>> j,
                               P pos) {
  assert(i != j);
  if constexpr (std::is_trivially_copyable_v<T>) {
    T new_j = *(heap + i);
    *(heap + i) = *(heap + j);
    pos(*(heap + i), i);
    *(heap + j) = new_j;
    pos(*(heap + j), j);
  } else {
    std::swap(*(heap + i), *(heap + j));
    pos(*(heap + i), i);
    pos(*(heap + j), j);
  }
}

template <typename T, typename C, typename P>
constexpr cbu::ByteSize<T*> heap_up(T* heap,
                                    cbu::ByteSize<std::type_identity_t<T>> k,
                                    C comp, P pos) {
  while (k && comp(*(heap + k), *(heap + (k - 1) / 2))) {
    auto parent = (k - 1) / 2;
    swap_pos(heap, k, parent, pos);
    k = parent;
  }
  return k;
}

template <typename T, typename C, typename P>
constexpr cbu::ByteSize<T> heap_down(T* heap,
                                     cbu::ByteSize<std::type_identity_t<T>> k,
                                     cbu::ByteSize<std::type_identity_t<T>> n,
                                     C comp, P pos) {
  while (k * 2 + 1 < n) {
    cbu::ByteSize<T> left = k * 2 + 1;
    cbu::ByteSize<T> right = left + 1;
    cbu::ByteSize<T> new_pos;
    if ((right < n) && comp(*(heap + right), *(heap + k))) {
      // right < self
      if (comp(*(heap + left), *(heap + right))) { // left < right < self
        new_pos = left;
      } else { // right < self; right <= left
        new_pos = right;
      }
    } else if (comp(*(heap + left), *(heap + k))) {  // left < self <= right
      new_pos = left;
    } else {
      break;
    }
    swap_pos(heap, k, new_pos, pos);
    k = new_pos;
  }
  return k;
}

template <typename T, typename C, typename P>
constexpr cbu::ByteSize<T*> heap_both(T* heap,
                                      cbu::ByteSize<std::type_identity_t<T>> k,
                                      cbu::ByteSize<std::type_identity_t<T>> n,
                                      C comp, P pos) {
  auto new_k = heap_up(heap, k, comp, pos);
  if (new_k == k) new_k = heap_down(heap, k, n, comp, pos);
  return new_k;
}

} // namespace heapq_detail

template <typename Vec>
concept Heapq_vector = requires(Vec &vec, std::size_t n) {
  typename Vec::value_type;
  { vec.size() } -> std::convertible_to<std::size_t>;
  { vec.clear() };
  { vec.resize(n) };
  { vec[n] } -> std::convertible_to<typename Vec::value_type &>;
};

namespace heapq_detail {

// Some of my own implementation provides shrink_to or truncate_to for shrinking
template <Heapq_vector Heap, typename Size>
inline constexpr void shrink_to(Heap& heap, Size n) {
  if constexpr (requires(Heap & heap, Size n) { {heap.shrink_to(n)}; }) {
    heap.shrink_to(n);
  } else if constexpr (requires(Heap & heap, Size n) {
                         {heap.truncate_to(n)};
                       }) {
    heap.truncate_to(n);
  } else {
    heap.resize(n);
  }
}

}  // namespace heapq_detail

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
constexpr void heapq_pop(Heap& heap, C comp = C(), P pos = P()) {
  auto n = heap.size();
  if (n <= 1) {
    heap.clear();
  } else {
    heap[0] = std::move(heap[n - 1]);
    pos(heap[0], 0);
    heapq_detail::shrink_to(heap, --n);
    heapq_detail::heap_down(&heap[0], 0, n, comp, pos);
  }
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
constexpr auto heapq_adjust_tail(Heap& heap, C comp = C(), P pos = P()) {
  assert(!heap.empty());
  return heapq_detail::heap_up(&heap[0], heap.size() - 1, comp, pos);
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
constexpr auto heapq_adjust_top(Heap& heap, C comp = C(), P pos = P()) {
  assert(!heap.empty());
  return heapq_detail::heap_down(&heap[0], 0, heap.size(), comp, pos);
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
constexpr auto heapq_adjust(Heap& heap,
                            cbu::ByteSize<typename Heap::value_type> k,
                            C comp = C(), P pos = P()) {
  return heapq_detail::heap_both(&heap[0], k, heap.size(), comp, pos);
}

// Remove an item from any position
template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
constexpr void heapq_remove(Heap& heap,
                            cbu::ByteSize<typename Heap::value_type> k,
                            C comp = C(), P pos = P()) {
  auto n = heap.size();
  assert(k < n);
  if (k < n - 1) {
    heap[k] = std::move(heap[n - 1]);
    pos(heap[k], k);
    heapq_detail::heap_both(&heap[0], k, n - 1, comp, pos);
  }
  heapq_detail::shrink_to(heap, n - 1);
}

template <typename T, typename C = std::less<>>
constexpr bool heap_verify(const T* heap,
                           cbu::ByteSize<std::type_identity_t<T>> n,
                           C comp = C()) noexcept {
  for (cbu::ByteSize<T> i = 0; i * 2 + 1 < n; ++i) {
    cbu::ByteSize<T> l = i * 2 + 1;
    cbu::ByteSize<T> r = i * 2 + 2;
    if (comp(*(heap + l), *(heap + i))) return false;
    if (r < n && comp(*(heap + r), *(heap + i))) return false;
  }
  return true;
}

template <typename T, typename C = std::less<>>
constexpr void heap_down(T* heap, cbu::ByteSize<std::type_identity_t<T>> i,
                         cbu::ByteSize<std::type_identity_t<T>> n,
                         C comp = C()) noexcept {
  while (true) {
    auto l = i * 2 + 1;
    auto r = i * 2 + 2;
    if (r > n /* l >= n */) break;
    auto next = i;
    if (r < n && comp(*(heap + r), *(heap + i))) {
      if (comp(*(heap + l), *(heap + r)))
        next = l;
      else
        next = r;
    } else if (comp(*(heap + l), *(heap + i))) {
      next = l;
    } else {
      break;
    }
    std::swap(*(heap + i), *(heap + next));
    i = next;
  }
}

template <typename T, typename C = std::less<>>
constexpr void heap_make(T* heap, cbu::ByteSize<std::type_identity_t<T>> n,
                         C comp = C()) noexcept {
  if (n < 2) return;
  auto i = (n - 2) / 2;
  do {
    heap_down(heap, i, n, comp);
  } while (i--);
}

template <typename T, typename C = std::less<>>
constexpr void heap_push(T* heap, cbu::ByteSize<std::type_identity_t<T>> n,
                         std::type_identity_t<T>&& value,
                         C comp = C()) noexcept {
  auto i = n;
  while (i) {
    auto p = (i - 1) / 2;
    if (comp(value, *(heap + p))) {
      *(heap + i) = std::move(*(heap + p));
      i = p;
    } else {
      break;
    }
  }
  *(heap + i) = std::move(value);
}

template <typename T, typename C = std::less<>>
constexpr void heap_push(T* heap, cbu::ByteSize<std::type_identity_t<T>> n,
                         const std::type_identity_t<T>& value,
                         C comp = C()) noexcept {
  auto i = n;
  while (i) {
    auto p = (i - 1) / 2;
    if (comp(value, *(heap + p))) {
      *(heap + i) = std::move(*(heap + p));
      i = p;
    } else {
      break;
    }
  }
  *(heap + i) = value;
}

template <typename T, typename C = std::less<>>
constexpr void heap_pop(T* heap, cbu::ByteSize<std::type_identity_t<T>> n,
                        C comp = C()) noexcept {
  T& value = heap[--n];
  if (n == 0) return;
  cbu::ByteSize<T> i = 0;
  while (i * 2 + 1 < n) {
    auto l = i * 2 + 1;
    auto r = i * 2 + 2;
    if (r < n && comp(*(heap + r), value) && !comp(*(heap + l), *(heap + r))) {
      *(heap + i) = std::move(*(heap + r));
      i = r;
    } else if (comp(*(heap + l), value)) {
      *(heap + i) = std::move(*(heap + l));
      i = l;
    } else {
      break;
    }
  }
  *(heap + i) = std::move(value);
}

} // namespace cbu
