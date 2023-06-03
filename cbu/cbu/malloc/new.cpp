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

#include "cbu/malloc/new.h"

#include <stdlib.h>

#include <new>

#include "cbu/alloc/alloc.h"
#include "cbu/alloc/pagesize.h"
#include "cbu/common/procutil.h"
#include "cbu/malloc/visibility.h"

namespace {

namespace alloc = cbu::alloc;
using cbu::operator""_lpsl;

void* new_nothrow(size_t n) noexcept {
  // C standard says malloc(0) may or may not return NULL
  // C++ standard says ::operator new(0) must reutrn a non-NULL pointer
  if (n == 0) n = 1;
  return alloc::allocate(n);
}

void* new_throw(size_t n) {
  void* r = new_nothrow(n);
#ifndef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
  if (r == nullptr)
    throw std::bad_alloc();
#endif
  return r;
}

}  // namespace

// Regular new/delete
cbu_malloc_visibility_default
void* operator new(size_t n) { return new_throw(n); }

cbu_malloc_visibility_default
void* operator new[](size_t n) { return new_throw(n); }

cbu_malloc_visibility_default
void operator delete(void* p) noexcept { alloc::reclaim(p); }

cbu_malloc_visibility_default
void operator delete[](void* p) noexcept { alloc::reclaim(p); }

// sized delete
cbu_malloc_visibility_default void operator delete(void* p,
                                                   size_t size) noexcept {
  alloc::reclaim(p, size);
}

cbu_malloc_visibility_default void operator delete[](void* p,
                                                     size_t size) noexcept {
  alloc::reclaim(p, size);
}

// nothrow new/delete
cbu_malloc_visibility_default
void* operator new(size_t n, const std::nothrow_t&) noexcept {
  return new_nothrow(n);
}

cbu_malloc_visibility_default
void* operator new[](size_t n, const std::nothrow_t&) noexcept {
  return new_nothrow(n);
}

cbu_malloc_visibility_default void operator delete(
    void* p, const std::nothrow_t&) noexcept {
  alloc::reclaim(p);
}

cbu_malloc_visibility_default void operator delete[](
    void* p, const std::nothrow_t&) noexcept {
  alloc::reclaim(p);
}

// aligned new/delete
cbu_malloc_visibility_default
void* operator new(size_t n, std::align_val_t alignment) {
  if (n == 0)
    n = 1;
  if (size_t(alignment) - 1 >= alloc::kPageSize) {
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
    cbu::fatal<"Invalid aligned allocation">();
#else
    throw std::bad_alloc();
#endif
  }
  void* ptr = alloc::allocate(
      n, alloc::AllocateOptions().with_align(size_t(alignment)));
  if (ptr == nullptr) {
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
    // Can fail because of invalid alignment
    cbu::fatal<"Invalid aligned allocation">();
#else
    throw std::bad_alloc();
#endif
  }
  return ptr;
}

cbu_malloc_visibility_default
void* operator new[] (size_t n, std::align_val_t align) {
  return operator new(n, align);
}

cbu_malloc_visibility_default
void operator delete(void* ptr, std::align_val_t) noexcept {
  alloc::reclaim(ptr);
}

cbu_malloc_visibility_default
void operator delete [] (void* ptr, std::align_val_t) noexcept {
  alloc::reclaim(ptr);
}

cbu_malloc_visibility_default
void* operator new(size_t n, std::align_val_t alignment,
                   const std::nothrow_t&) noexcept {
  if (n == 0) n = 1;
  if (size_t(alignment) - 1 >= alloc::kPageSize) return nullptr;
  return alloc::allocate(
      n, alloc::AllocateOptions().with_align(size_t(alignment)));
}

cbu_malloc_visibility_default
void* operator new[](size_t n, std::align_val_t alignment,
                     const std::nothrow_t& nothrow) noexcept {
  return operator new(n, alignment, nothrow);
}

cbu_malloc_visibility_default
void operator delete(void* ptr, size_t size, std::align_val_t) noexcept {
  alloc::reclaim(ptr, size);
}

cbu_malloc_visibility_default
void operator delete [] (void* ptr, size_t size, std::align_val_t) noexcept {
  alloc::reclaim(ptr, size);
}

cbu_malloc_visibility_default void operator delete(
    void* p, std::align_val_t, const std::nothrow_t&) noexcept {
  alloc::reclaim(p);
}

cbu_malloc_visibility_default void operator delete[](
    void* p, std::align_val_t, const std::nothrow_t&) noexcept {
  alloc::reclaim(p);
}

namespace cbu {

void* new_realloc(void* ptr, size_t n) {
  if (n == 0)
    n = 1;
  ptr = alloc::reallocate(ptr, n);
#ifndef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
  if (ptr == NULL)
    throw std::bad_alloc();
#endif
  return ptr;
}

void* new_aligned_realloc(void* ptr, size_t n, size_t alignment) {
  if (n == 0) n = 1;
  if (size_t(alignment) - 1 >= alloc::kPageSize) {
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
    cbu::fatal<"Invalid aligned allocation">();
#else
    throw std::bad_alloc();
#endif
  }
  ptr = alloc::reallocate(
      ptr, n, alloc::AllocateOptions().with_align(size_t(alignment)));
  if (ptr == nullptr) {
#ifdef CBU_ASSUME_MEMORY_ALLOCATION_NEVER_FAILS
    // Can fail because of invalid alignment
    cbu::fatal<"Invalid aligned allocation">();
#else
    throw std::bad_alloc();
#endif
  }
  return ptr;
}

}  // namespace cbu
