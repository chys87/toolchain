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

#include "cbu/alloc/alloc.h"

#include <string.h>

#include <algorithm>

#include "cbu/alloc/pagesize.h"
#include "cbu/alloc/private.h"

namespace cbu {
namespace alloc {

void* allocate(size_t size, AllocateOptions options) noexcept {
  size_t boundary = options.align;
  if (boundary) {
    if (boundary > kPageSize) return nullptr;
    if (boundary & (boundary - 1)) return nullptr;
    size = (size + boundary - 1) & ~(boundary - 1);
    // Once size is properly adjuted, our allocation design guarantees proper
    // alignment.
  }

  void* ptr = nullptr;

  if (size > category_to_size(kMaxCategory)) {
    ptr = alloc_large(size, options.zero);
    if (false_no_fail(ptr == nullptr)) return nomem();
  } else if (size != 0) {
    ptr = alloc_small(size);
    if (false_no_fail(ptr == nullptr)) return nomem();
    if (options.zero) ptr = memset(ptr, 0, size);
  }
  return ptr;
}

void* reallocate(void* ptr, size_t new_size, AllocateOptions options) noexcept {
  size_t boundary = options.align;
  if (boundary) {
    if (boundary > kPageSize) return nullptr;
    if (boundary & (boundary - 1)) return nullptr;
    new_size = (new_size + boundary - 1) & ~(boundary - 1);
  }

  if (new_size == 0) {
    reclaim(ptr);
    return nullptr;
  } else if (uintptr_t(ptr) % kPageSize) {  // Was small block.
    unsigned old_cat = small_allocated_category(ptr);
    size_t old_size = category_to_size(old_cat);
    size_t copy_size;
    void* nptr;
    if (new_size <= category_to_size(kMaxCategory)) {
      // Small to small.
      unsigned new_cat = size_to_category(new_size);
      if (old_cat == new_cat) return ptr;
      nptr = alloc_small_category(new_cat);
      copy_size = std::min(old_size, new_size);
    } else {
      // Small to large
      nptr = alloc_large(new_size, false);
      copy_size = old_size;
    }
    // Copy
    if (true_no_fail(nptr)) {
      nptr = memcpy(nptr, ptr, copy_size);
      free_small(ptr);
    }
    return nptr;
  } else if (ptr == nullptr) {
    return allocate(new_size, options);
  } else {  // Was large block.
    if (new_size <= kSmallAllocLimit) {
      void* nptr = alloc_small(new_size);
      if (true_no_fail(nptr)) {
        nptr = memcpy(nptr, ptr, new_size);
        free_large(ptr);
      }
      return nptr;
    } else {
      return realloc_large(ptr, new_size);
    }
  }
}

void reclaim(void* ptr) noexcept {
  if (uintptr_t(ptr) % kPageSize)  // Not aligned. Small blocks.
    free_small(ptr);
  else if (ptr)  // Aligned. Large block.
    free_large(ptr);
}

void reclaim(void* ptr, size_t size) noexcept {
  // We must be very careful:
  // size might be much smaller than our actually allocated size
  // if the memory block was allocated with an alignment option
  if (uintptr_t(ptr) % kPageSize)
    free_small(ptr, size);
  else if (ptr)
    free_large(ptr, size);
}

size_t allocated_size(void* ptr) noexcept {
  if (uintptr_t(ptr) % kPageSize)
    return small_allocated_size(ptr);
  else if (ptr == nullptr)
    return 0;
  else
    return large_allocated_size(ptr);
}

void trim(size_t pad) noexcept {
  small_trim(pad);
  large_trim(pad);
}

}  // namespace alloc
}  // namespace cbu
