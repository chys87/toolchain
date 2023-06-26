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

#include <errno.h>
#include <stdlib.h>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/pagesize.h"
#include "cbu/malloc/malloc.h"
#include "cbu/malloc/visibility.h"
#include "cbu/math/strict_overflow.h"

namespace alloc = cbu::alloc;

extern "C" void* cbu_malloc(size_t n) noexcept { return alloc::allocate(n); }

extern "C" void* cbu_calloc(size_t m, size_t l) noexcept {
  size_t n;
  if (cbu::mul_overflow(m, l, &n)) return alloc::nomem();
  return alloc::allocate(n, alloc::AllocateOptions().with_zero(true));
}

extern "C" void* cbu_realloc(void* ptr, size_t newsize) noexcept {
  return alloc::reallocate(ptr, newsize);
}

extern "C" void* cbu_reallocarray(void* ptr, size_t m, size_t l) noexcept {
  size_t n;
  if (cbu::mul_overflow(m, l, &n)) return alloc::nomem();
  return alloc::reallocate(ptr, n);
}

extern "C" void* cbu_memalign(size_t boundary, size_t n) noexcept {
  // Check boundary so that it always fits in AllocateOptions::align
  if (boundary - 1 >= alloc::kPageSize) return nullptr;
  return alloc::allocate(n, alloc::AllocateOptions().with_align(boundary));
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
extern "C" void* cbu_aligned_alloc(size_t boundary, size_t n) noexcept
    __attribute__((alias("cbu_memalign")));

extern "C" void* cbu_valloc(size_t n) noexcept {
  // Don't directly call alloc_large, which doesn't like n == 0
  return cbu_memalign(alloc::kPageSize, n);
}

extern "C" void* cbu_pvalloc(size_t) noexcept
  __attribute__((alias("cbu_valloc"), malloc));

extern "C" void cbu_free(void* ptr) noexcept { alloc::reclaim(ptr); }

extern "C" void cbu_cfree(void* ) noexcept __attribute__((alias("cbu_free")));

extern "C" int cbu_posix_memalign(void** pret, size_t boundary,
                                  size_t n) noexcept {
  if (boundary & (boundary - 1))
    return EINVAL;
  if (boundary < sizeof(void*))  // POSIX so requires
    return EINVAL;
  void* ptr = cbu_memalign(boundary, n);
  *pret = ptr;
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
  return 0;
#else
  return ptr ? 0 : ENOMEM;
#endif
}

extern "C" size_t cbu_malloc_usable_size(void* ptr) noexcept {
  return alloc::allocated_size(ptr);
}

extern "C" int cbu_malloc_trim(size_t pad) noexcept {
#ifdef CBU_NEED_MALLOC_TRIM
  alloc::trim(pad);
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
int posix_memalign(void**, size_t, size_t) noexcept
    __attribute__((alias("cbu_posix_memalign"))) cbu_malloc_visibility_default;
size_t malloc_usable_size(void*) noexcept
  __attribute__((alias("cbu_malloc_usable_size"), pure, cold))
  cbu_malloc_visibility_default;
int malloc_trim(size_t) noexcept
  __attribute__((alias("cbu_malloc_trim"), cold))
  cbu_malloc_visibility_default;

} // extern "C"
