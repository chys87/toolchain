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

#include <stddef.h>
#include "cbu/malloc/visibility.h"

extern "C" {

// Forward declarations. Don't declare their standard names
void *cbu_malloc(size_t) noexcept
  __attribute__((__malloc__)) cbu_malloc_visibility_default;

void cbu_free(void *) noexcept cbu_malloc_visibility_default;

void cbu_cfree(void *) noexcept cbu_malloc_visibility_default;

void cbu_sized_free(void *, size_t) noexcept cbu_malloc_visibility_default;

void *cbu_calloc(size_t, size_t) noexcept
  __attribute__((__malloc__)) cbu_malloc_visibility_default;

void *cbu_realloc(void *, size_t) noexcept cbu_malloc_visibility_default;

void *cbu_reallocarray(void *, size_t, size_t) noexcept
  cbu_malloc_visibility_default;

void *cbu_aligned_realloc(void *, size_t, size_t) noexcept
  cbu_malloc_visibility_default;

void *cbu_aligned_alloc (size_t, size_t) noexcept
  __attribute__((__malloc__)) cbu_malloc_visibility_default;

void *cbu_memalign(size_t, size_t) noexcept
  __attribute__((__malloc__)) cbu_malloc_visibility_default;

void *cbu_valloc(size_t) noexcept
  __attribute__((__cold__, __malloc__)) cbu_malloc_visibility_default;

void *cbu_pvalloc(size_t) noexcept
  __attribute__((__cold__, __malloc__)) cbu_malloc_visibility_default;

int cbu_posix_memalign(void **, size_t, size_t) noexcept
  __attribute__((__cold__, __nonnull__(1))) cbu_malloc_visibility_default;

size_t cbu_malloc_usable_size(void *) noexcept
  __attribute__((__cold__, __pure__)) cbu_malloc_visibility_default;

int cbu_malloc_trim(size_t) noexcept
#ifndef CBU_NEED_MALLOC_TRIM
  __attribute__((__cold__))
#endif
  cbu_malloc_visibility_default
  ;

} // extern "C"
