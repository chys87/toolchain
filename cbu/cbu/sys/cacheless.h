/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021-2023, chys <admin@CHYS.INFO>
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

// This file provides utilities to write to memory bypassing cache.
// Don't use this unless you know what you're doing.
//
// Currently this is only implemented for x86-64 (i386 also supports some
// of the features, but nobody uses it nowadays.)

#pragma once

#include <stdint.h>
#include <stddef.h>
#if defined __i386__ || defined __x86_64__
# include <x86intrin.h>
#endif
#if defined __aarch64__ && defined __ARM_NEON
# include <arm_neon.h>
#endif

#include "cbu/strings/faststr.h"

namespace cbu {
namespace cacheless {

#ifdef __aarch64__
template <int offset = 0>
inline void* store2(void* dst, uint32_t a, uint32_t b) noexcept {
  // Can't use "m" constraint, stnp not supporting all addressings
  asm("stnp %w1, %w2, [%0, %3]" :: "r"(dst), "r"(a), "r"(b), "i"(offset) : "memory");
  return static_cast<char*>(dst) + 8;
}

template <int offset = 0>
inline void* store2(void* dst, uint64_t a, uint64_t b) noexcept {
  // Can't use "m" constraint, stnp not supporting all addressings
  asm("stnp %1, %2, [%0, %3]" :: "r"(dst), "r"(a), "r"(b), "i"(offset) : "memory");
  return static_cast<char*>(dst) + 16;
}
#endif

inline void* store(void* dst, uint32_t value) noexcept {
#ifdef __SSE2__  // MOVNTI was introduced as part of SSE2
  _mm_stream_si32(static_cast<int*>(dst), value);
  return static_cast<char*>(dst) + 4;
#else
  return memdrop(static_cast<char*>(dst), value);
#endif
}

inline void* store(void* dst, uint64_t value) noexcept {
#ifdef __x86_64__
  _mm_stream_si64(static_cast<long long*>(dst), value);
  return static_cast<char*>(dst) + 8;
#elif defined __aarch64__
  return store2(dst, uint32_t(value), uint32_t(value >> 32));
#else
  return memdrop(static_cast<char*>(dst), value);
#endif
}

#ifdef __SSE2__
// dst must be properly aligned
inline void* store(void* dst, __m128i value) noexcept {
  _mm_stream_si128(static_cast<__m128i*>(dst), value);
  return static_cast<char*>(dst) + 16;
}
#endif

#ifdef __AVX__
inline void* store(void* dst, __m256i value) noexcept {
  _mm256_stream_si256(static_cast<__m256i*>(dst), value);
  return static_cast<char*>(dst) + 32;
}
#endif

void* copy(void* dst, const void* src, size_t size) noexcept;

// dst must be aligned to sizeof(value) bytes
void* fill(void* dst, uint8_t value, size_t size) noexcept;
void* fill(void* dst, uint16_t value, size_t size) noexcept;
void* fill(void* dst, uint32_t value, size_t size) noexcept;
void* fill(void* dst, uint64_t value, size_t size) noexcept;

inline void fence() noexcept {
#if defined __i386__ || defined __x86_64__
  _mm_sfence();
#endif
  // It appears we don't need to issue fence instructions for aarch64.
  // Non-temporal stores have no memory-order implication on aarch64.
}

} // namespace cacheless
} // namespace cbu
