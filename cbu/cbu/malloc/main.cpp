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

#include "cbu/malloc/malloc.h"
#include "cbu/malloc/permanent.h"
#include "cbu/malloc/private.h"
#include "cbu/malloc/pagesize.h"
#include "cbu/malloc/visibility.h"
#include "cbu/common/fastarith.h"
#include <string.h>
#include <unistd.h>
#include <algorithm>

namespace cbu {
namespace malloc_details {
namespace {

// Naive realloc, for debugging purpose only
__attribute__((unused))
void* naive_realloc(void* ptr, size_t newsize) {
  if (newsize == 0) {
    cbu_free(ptr);
    return nullptr;
  } else if (ptr == nullptr) {
    return cbu_malloc(newsize);
  } else {
    size_t oldsize = (uintptr_t(ptr) % pagesize) ?
      small_allocated_size(ptr) :
      large_allocated_size(ptr);
    void* nptr = cbu_malloc(newsize);
    memcpy(nptr, ptr, std::min(oldsize, newsize));
    cbu_free(ptr);
    return nptr;
  }
}

}  // namespace
}  // namespace cbu_malloc
}  // namespace cbu

using namespace cbu::malloc_details;

extern "C" void* cbu_malloc(size_t n) noexcept {
  void* ptr;
  if (n > category_to_size(max_category)) {
    ptr = alloc_large(n);
  } else if (n == 0) {
    return nullptr;
  } else {
    ptr = alloc_small(n);
  }
  if (false_no_fail(ptr == nullptr))
    return nomem();
  return ptr;
}

extern "C" void* cbu_calloc(size_t m, size_t l) noexcept {
  size_t n;
  if (cbu::mul_overflow(m, l, &n))
    return nomem();
  void* ptr = cbu_malloc(n);
  if (false_no_fail(ptr == nullptr))
    return nomem();
  return memset(ptr, 0, n);
}

extern "C"
[[gnu::noinline]]
void* cbu_realloc(void* ptr, size_t newsize) noexcept {
  if (newsize == 0) {
    cbu_free(ptr);
    return nullptr;
  } else if (uintptr_t(ptr) % pagesize) { // Was small block.
    unsigned oldcat = small_allocated_category(ptr);
    size_t oldsize = category_to_size(oldcat);
    size_t copysize;
    void* nptr;
    if (newsize <= category_to_size(max_category)) {
      // Small to small.
      unsigned newcat = size_to_category(newsize);
      if (oldcat == newcat)
        return ptr;
      nptr = alloc_small_category(newcat);
      copysize = std::min(oldsize, newsize);
    } else {
      // Small to large
      nptr = alloc_large(newsize);
      copysize = oldsize;
    }
    // Copy
    if (true_no_fail(nptr)) {
      nptr = memcpy(nptr, ptr, copysize);
      free_small(ptr);
    }
    return nptr;
  } else if (ptr == nullptr) {
    return cbu_malloc(newsize);
  } else { // Was large block.
    if (newsize <= small_alloc_limit) {
      void* nptr = alloc_small(newsize);
      if (true_no_fail(nptr)) {
        nptr = memcpy(nptr, ptr, newsize);
        free_large(ptr);
      }
      return nptr;
    } else {
      return realloc_large(ptr, newsize);
    }
  }
}

extern "C" void* cbu_reallocarray(void* ptr, size_t m, size_t l) noexcept {
  size_t n;
  if (cbu::mul_overflow(m, l, &n))
    return nomem();
  return cbu_realloc(ptr, n);
}

extern "C" void* cbu_aligned_realloc(void* ptr, size_t n,
                                     size_t boundary) noexcept {
  if (boundary - 1 >= pagesize)
    return nullptr;
  if (boundary & (boundary - 1))
    return nullptr;
  n = (n + boundary - 1) & ~(boundary - 1);
  // Our allocation design guarantees proper alignment.
  return cbu_realloc(ptr, n);
}

extern "C" void* cbu_memalign(size_t boundary, size_t n) noexcept {
  if (boundary - 1 >= pagesize)
    return nullptr;
  if (boundary & (boundary - 1))
    return nullptr;
  n = (n + boundary - 1) & ~(boundary - 1);
  // Our allocation design guarantees proper alignment.
  return cbu_malloc(n);
}

// C11 says the boundary must be "supported by the implementation", so we
// can determine it: It must be nonzero, nor can it be larger than
// pagesize, and it must be power of 2.
// C11 does not require setting errno, and glibc doesn't do so,
// so neither do we.
// C11 also requires n to be a integral multiple of boundary, but both
// glibc people and I think it's moronic and decide to neglect it.
// And now, we find that aligned_alloc is identical to our memalign in
// every aspect..
extern "C" void* cbu_aligned_alloc (size_t boundary, size_t n) noexcept
  __attribute__((alias("cbu_memalign")));

extern "C" void* cbu_valloc(size_t n) noexcept {
  // Don't directly call alloc_large, which doesn't like n == 0
  return cbu_memalign(pagesize, n);
}

extern "C" void* cbu_pvalloc(size_t) noexcept
  __attribute__((alias("cbu_valloc"), malloc));

extern "C"
[[gnu::noinline]]
void cbu_free(void* ptr) noexcept {
  if (uintptr_t(ptr) % pagesize) // Not aligned. Small blocks.
    free_small(ptr);
  else if (ptr) // Aligned. Large block.
    free_large(ptr);
}

extern "C" void cbu_cfree(void* ) noexcept __attribute__((alias("cbu_free")));

extern "C"
[[gnu::noinline]]
void cbu_sized_free(void* ptr, size_t size) noexcept {
  // We must be very careful:
  // size might be much smaller than our actually allocated size
  // if the memory block was allocated with memalign
  if (uintptr_t(ptr) % pagesize) {
    // Small blocks. We can't depend on size because of memalign
    free_small(ptr, size);
  } else if (ptr) {
    free_large(ptr, size);
  }
}

extern "C" int cbu_posix_memalign(void* *pret, size_t boundary,
                                  size_t n) noexcept {
  if (boundary & (boundary - 1))
    return EINVAL;
  if (boundary < sizeof(void* )) // POSIX so requires
    return EINVAL;
  void* ptr = cbu_memalign(boundary, n);
  *pret = ptr;
  return true_no_fail(ptr) ? 0 : ENOMEM;
}

extern "C" size_t cbu_malloc_usable_size(void* ptr) noexcept {
  if (uintptr_t(ptr) % pagesize)
    return small_allocated_size(ptr);
  else if (ptr == nullptr)
    return 0;
  else
    return large_allocated_size(ptr);
}

extern "C" int cbu_malloc_trim(size_t pad) noexcept {
#ifdef CBU_NEED_MALLOC_TRIM
  small_trim(pad);
  large_trim(pad);
  return 1;
#else
  return 0;
#endif
}

extern "C" {

void* malloc(size_t) noexcept
  __attribute__((alias("cbu_malloc"), malloc)) cbu_malloc_visibility_default;
void free(void*) noexcept
  __attribute__((alias("cbu_free"))) cbu_malloc_visibility_default;
void cfree(void*) noexcept
  __attribute__((alias("cbu_cfree"))) cbu_malloc_visibility_default;
void* calloc(size_t, size_t) noexcept
  __attribute__((alias("cbu_calloc"))) cbu_malloc_visibility_default;
void* realloc(void*, size_t) noexcept
  __attribute__((alias("cbu_realloc"))) cbu_malloc_visibility_default;
void* reallocarray(void*, size_t, size_t) noexcept
  __attribute__((alias("cbu_reallocarray"))) cbu_malloc_visibility_default;
void* aligned_alloc(size_t, size_t) noexcept
  __attribute__((alias("cbu_aligned_alloc"))) cbu_malloc_visibility_default;
void* memalign(size_t, size_t) noexcept
  __attribute__((alias("cbu_memalign"), malloc)) cbu_malloc_visibility_default;
void* valloc(size_t) noexcept
  __attribute__((alias("cbu_valloc"), malloc, cold))
  cbu_malloc_visibility_default;
void* pvalloc(size_t) noexcept
  __attribute__((alias("cbu_pvalloc"), malloc, cold))
  cbu_malloc_visibility_default;
int posix_memalign (void**, size_t, size_t) noexcept
  __attribute__((alias("cbu_posix_memalign"), cold))
  cbu_malloc_visibility_default;
size_t malloc_usable_size(void*) noexcept
  __attribute__((alias("cbu_malloc_usable_size"), pure, cold))
  cbu_malloc_visibility_default;
int malloc_trim(size_t) noexcept
  __attribute__((alias("cbu_malloc_trim"), cold))
  cbu_malloc_visibility_default;

} // extern "C"
