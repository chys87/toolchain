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

#include "cbu/alloc/alloc.h"

#include <string.h>

#include <algorithm>

#include "cbu/alloc/pagesize.h"
#include "cbu/alloc/private/common.h"
#include "cbu/common/procutil.h"
#include "cbu/strings/faststr.h"

namespace cbu {
namespace alloc {

constexpr bool is_alignment_valid_impl(size_t align) noexcept {
  // return align && (align <= kPageSize) && ((align & (align - 1)) == 0);
  return ((align - 1) & (align | ~size_t(kPageSize - 1))) == 0;
}

constexpr bool is_alignment_valid_posix_impl(size_t align) noexcept {
  // Equivalent to align >= sizeof(void*) and is_alignment_valid(align)
  // posix_memalign requires align >= sizeof(void*)
  return ((align - sizeof(void*)) & (align | ~size_t(kPageSize - sizeof(void*)))) == 0;
}

static_assert(!is_alignment_valid_impl(0));
static_assert(is_alignment_valid_impl(1));
static_assert(is_alignment_valid_impl(2));
static_assert(!is_alignment_valid_impl(3));
static_assert(is_alignment_valid_impl(4));
static_assert(!is_alignment_valid_impl(5));
static_assert(is_alignment_valid_impl(16));
static_assert(!is_alignment_valid_impl(kPageSize - 1));
static_assert(is_alignment_valid_impl(kPageSize));
static_assert(!is_alignment_valid_impl(kPageSize + 1));
static_assert(!is_alignment_valid_impl(kPageSize + 2));
static_assert(!is_alignment_valid_impl(kPageSize * 2));
static_assert(!is_alignment_valid_impl(kPageSize * 3));
static_assert(!is_alignment_valid_impl(kPageSize * 4));

static_assert(!is_alignment_valid_posix_impl(0));
static_assert(!is_alignment_valid_posix_impl(1));
static_assert(!is_alignment_valid_posix_impl(2));
static_assert(!is_alignment_valid_posix_impl(3));
static_assert(is_alignment_valid_posix_impl(4) == (sizeof(void*) <= 4));
static_assert(!is_alignment_valid_posix_impl(5));
static_assert(is_alignment_valid_posix_impl(16));
static_assert(!is_alignment_valid_posix_impl(kPageSize - 1));
static_assert(is_alignment_valid_posix_impl(kPageSize));
static_assert(!is_alignment_valid_posix_impl(kPageSize + 1));
static_assert(!is_alignment_valid_posix_impl(kPageSize + 2));
static_assert(!is_alignment_valid_posix_impl(kPageSize * 2));
static_assert(!is_alignment_valid_posix_impl(kPageSize * 3));
static_assert(!is_alignment_valid_posix_impl(kPageSize * 4));


bool is_alignment_valid(size_t align) noexcept {
  return is_alignment_valid_impl(align);
}

bool is_alignment_valid_posix(size_t align) noexcept {
  return is_alignment_valid_posix_impl(align);
}

void* allocate(size_t size, AllocateOptions options) noexcept {
  if (size_t boundary = options.align) {
    size = (size + boundary - 1) & ~(boundary - 1);
    // Once size is properly adjusted, our allocation design guarantees proper
    // alignment.
  }

  void* ptr = nullptr;

  if (size > kSmallAllocLimit) {
    ptr = alloc_large(size, options.zero);
    if (false_no_fail(ptr == nullptr)) return nomem();
  } else if (size != 0) {
    if (__builtin_constant_p(size)) { // For LTO
      ptr = alloc_small_category(size_to_category(size));
    } else {
      ptr = alloc_small(size);
    }
    if (false_no_fail(ptr == nullptr)) return nomem();
    if (options.zero) ptr = memset_no_builtin(ptr, 0, size);
  }
  return ptr;
}

void* allocate(size_t size) noexcept {
  void* ptr = nullptr;

  if (size > kSmallAllocLimit) {
    ptr = alloc_large(size, false /* zero */);
    if (false_no_fail(ptr == nullptr)) return nomem();
  } else if (size != 0) {
    if (__builtin_constant_p(size) ||
        __builtin_constant_p(size_to_category(size))) {  // For LTO
      ptr = alloc_small_category(size_to_category(size));
    } else {
      ptr = alloc_small(size);
    }
    if (false_no_fail(ptr == nullptr)) return nomem();
  }
  return ptr;
}

void* reallocate(void* ptr, size_t new_size, AllocateOptions options) noexcept {
  if (size_t boundary = options.align) {
    new_size = (new_size + boundary - 1) & ~(boundary - 1);
  }

  if (new_size == 0) [[unlikely]] {
    reclaim(ptr);
    return nullptr;
  } else if (uintptr_t(ptr) % kPageSize) {  // Was small block.
    unsigned old_cat = small_allocated_category(ptr);
    size_t old_size = category_to_size(old_cat);
    size_t copy_size;
    void* nptr;
    if (new_size <= kSmallAllocLimit) {
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
      nptr = memcpy_no_builtin(nptr, ptr, copy_size);
      free_small(ptr);
    }
    return nptr;
  } else if (ptr == nullptr) {
    return allocate(new_size, options.with_align(0).with_zero(false));
  } else {  // Was large block.
    if (new_size <= kSmallAllocLimit) {
      void* nptr = alloc_small(new_size);
      if (true_no_fail(nptr)) {
        nptr = memcpy_no_builtin(nptr, ptr, new_size);
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
