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

#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace cbu {
inline namespace cbu_heapq {
namespace heapq_detail {

// Can be used to store the index into the positioned object
struct HeapDefaultPositioner {
  template <typename T>
  void operator()(T &, std::size_t) const {
  }
};

// Fallback implementation
template <typename T, typename P>
inline void swap_pos_impl(T *heap, std::size_t i, std::size_t j, P pos, float) {
  assert(i != j);
  std::swap(heap[i], heap[j]);
  pos(heap[i], i);
  pos(heap[j], j);
}

template <typename T, typename P>
requires std::is_trivially_copyable<T>::value
inline void swap_pos_impl(T *heap, std::size_t i, std::size_t j, P pos, int) {
  assert(i != j);
  T new_j = heap[i];
  heap[i] = heap[j];
  pos(heap[i], i);
  heap[j] = new_j;
  pos(heap[j], j);
}

template <typename T, typename P>
inline void swap_pos(T *heap, std::size_t i, std::size_t j, P pos) {
  swap_pos_impl(heap, i, j, pos, 0);
}

template<typename T, typename C, typename P>
std::size_t heap_up(T *heap, std::size_t k, C comp, P pos) {
  while (k && comp(heap[k], heap[(k - 1) / 2])) {
    std::size_t parent = (k - 1) / 2;
    swap_pos(heap, k, parent, pos);
    k = parent;
  }
  return k;
}

template<typename T, typename C, typename P>
std::size_t heap_down(T *heap, std::size_t k, std::size_t n, C comp, P pos) {
  while (k * 2 + 1 < n) {
    std::size_t left = k * 2 + 1;
    std::size_t right = left + 1;
    std::size_t new_pos;
    if ((right < n) && comp(heap[right], heap[k])) {
      // right < self
      if (comp(heap[left], heap[right])) { // left < right < self
        new_pos = left;
      } else { // right < self; right <= left
        new_pos = right;
      }
    } else if (comp(heap[left], heap[k])) { // left < self <= right
      new_pos = left;
    } else {
      break;
    }
    swap_pos(heap, k, new_pos, pos);
    k = new_pos;
  }
  return k;
}

template<typename T, typename C, typename P>
std::size_t heap_both(T *heap, std::size_t k, std::size_t n, C comp, P pos) {
  std::size_t new_k = heap_up(heap, k, comp, pos);
  if (new_k == k)
    new_k = heap_down(heap, k, n, comp, pos);
  return new_k;
}

} // namespace heapq_detail

template <typename Vec>
concept Heapq_vector = requires(Vec &vec, std::size_t n) {
  typename Vec::value_type;
  { vec.resize(n) };
  { vec[n] } -> std::convertible_to<typename Vec::value_type &>;
};

namespace heapq_detail {

// Some of my own implementation provides shrink_to for shrinking
template <typename Vec>
concept Heapq_vector_shrink_to = requires(Vec &vec, std::size_t n) {
  requires Heapq_vector<Vec>;
  vec.shrink_to(n);
};

template <Heapq_vector_shrink_to Heap>
inline void shrink_to(Heap &heap, std::size_t n) {
  heap.shrink_to(n);
}

template <Heapq_vector Heap>
requires (!Heapq_vector_shrink_to<Heap>)
inline void shrink_to(Heap &heap, std::size_t n) {
  heap.resize(n);
}

} // namespace heapq_detail

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
void heapq_pop(Heap &heap, C comp = C(), P pos = P()) {
  std::size_t n = heap.size();
  if (n >= 1) {
    if (n == 1) {
      heap.clear();
    } else {
      heap[0] = std::move(heap[n - 1]);
      pos(heap[0], 0);
      heapq_detail::shrink_to(heap, --n);
      heapq_detail::heap_down(&heap[0], 0, n, comp, pos);
    }
  }
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
std::size_t heapq_adjust_tail(Heap &heap, C comp = C(), P pos = P()) {
  assert(!heap.empty());
  return heapq_detail::heap_up(&heap[0], heap.size() - 1, comp, pos);
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
std::size_t heapq_adjust_top(Heap &heap, C comp = C(), P pos = P()) {
  assert(!heap.empty());
  return heapq_detail::heap_down(&heap[0], 0, heap.size(), comp, pos);
}

template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
std::size_t heapq_adjust(Heap &heap, std::size_t k, C comp = C(), P pos = P()) {
  return heapq_detail::heap_both(&heap[0], k, heap.size(), comp, pos);
}

// Remove an item from any position
template <Heapq_vector Heap, typename C = std::less<>,
          typename P = heapq_detail::HeapDefaultPositioner>
void heapq_remove(Heap &heap, std::size_t k, C comp = C(), P pos = P()) {
  std::size_t n = heap.size();
  assert(k < n);
  if (k < n - 1) {
    heap[k] = std::move(heap[n - 1]);
    pos(heap[k], k);
    heapq_detail::heap_both(&heap[0], k, n - 1, comp, pos);
  }
  heapq_detail::shrink_to(heap, n - 1);
}

} // inline namespace cbu_heapq
} // namespace cbu
