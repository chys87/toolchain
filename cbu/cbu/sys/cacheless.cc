/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021, chys <admin@CHYS.INFO>
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

#include "cbu/sys/cacheless.h"

#include <string.h>

#include "cbu/common/faststr.h"
#include "cbu/compat/string.h"

namespace cbu {
namespace cacheless {

#ifdef __x86_64__

void* store(void* dst, uint32_t value) noexcept {
  _mm_stream_si32(static_cast<int*>(dst), value);
  return static_cast<char*>(dst) + 4;
}

void* store(void* dst, uint64_t value) noexcept {
  _mm_stream_si64(static_cast<long long*>(dst), value);
  return static_cast<char*>(dst) + 8;
}

void* store(void* dst, __m128i value) noexcept {
  _mm_stream_si128(static_cast<__m128i*>(dst), value);
  return static_cast<char*>(dst) + 16;
}

#ifdef __AVX__
void* store(void* dst, __m256i value) noexcept {
  _mm256_stream_si256(static_cast<__m256i*>(dst), value);
  return static_cast<char*>(dst) + 32;
}
#endif

namespace {

// size is between 0 and 16
void* copy_le16(void* dst, const void* src, size_t size) noexcept {
  char* d = static_cast<char*>(dst);
  const char* s = static_cast<const char*>(src);

  if (size >= 8) {
    store(d, mempick8(s));
    if (size > 8) {
      store(d + size - 8, mempick8(s + size - 8));
    }
  } else if (size >= 4) {
    store(d, mempick4(s));
    if (size > 4) {
      store(d + size - 4, mempick4(s + size - 4));
    }
  } else if (size > 0){
    d[0] = s[0];
    d[size >> 1] = s[size >> 1];
    d[size - 1] = s[size - 1];
  }
  return d + size;
}

} // namespace

void* copy(void* dst, const void* src, size_t size) noexcept {
  if (size <= 16)
    return copy_le16(dst, src, size);

  char* d = static_cast<char*>(dst);
  const char* s = static_cast<const char*>(src);

  // movntdq requires aligned memory, so we use movnti
  store(d, mempick8(s));
  store(d + 8, mempick8(s + 8));

  uintptr_t misalign = uintptr_t(d) & 15;
  d += 16 - misalign;
  s += 16 - misalign;
  size -= 16 - misalign;

  if (size >= 512) {
    _mm_prefetch(s + 64, _MM_HINT_T0);
    _mm_prefetch(s + 128, _MM_HINT_T0);
    _mm_prefetch(s + 192, _MM_HINT_T0);
    while (size >= 64) {
      _mm_prefetch(s + 256, _MM_HINT_T0);
      _mm_stream_si128((__m128i*)d, *(const __m128i_u*)s);
      _mm_stream_si128((__m128i*)(d + 16), *(const __m128i_u*)(s + 16));
      _mm_stream_si128((__m128i*)(d + 32), *(const __m128i_u*)(s + 32));
      _mm_stream_si128((__m128i*)(d + 48), *(const __m128i_u*)(s + 48));
      d += 64;
      s += 64;
      size -= 64;
    }
  } else {
    while (size >= 64) {
      _mm_stream_si128((__m128i*)d, *(const __m128i_u*)s);
      _mm_stream_si128((__m128i*)(d + 16), *(const __m128i_u*)(s + 16));
      _mm_stream_si128((__m128i*)(d + 32), *(const __m128i_u*)(s + 32));
      _mm_stream_si128((__m128i*)(d + 48), *(const __m128i_u*)(s + 48));
      d += 64;
      s += 64;
      size -= 64;
    }
  }
  if (size & 32) {
    _mm_stream_si128((__m128i*)d, *(const __m128i_u*)s);
    _mm_stream_si128((__m128i*)(d + 16), *(const __m128i_u*)(s + 16));
    d += 32;
    s += 32;
    size -= 32;
  }
  if (size & 16) {
    _mm_stream_si128((__m128i*)d, *(const __m128i_u*)s);
    d += 16;
    s += 16;
    size -= 16;
  }
  if (size) {
    store(d + size - 16, mempick8(s + size - 16));
    store(d + size - 8, mempick8(s + size - 8));
  }
  return d + size;
}

void fence() noexcept {
  _mm_sfence();
}

#else

void* store(void* dst, uint32_t value) noexcept {
  return memdrop(static_cast<char*>(dst), value);
}

void* store(void* dst, uint64_t value) noexcept {
  return memdrop(static_cast<char*>(dst), value);
}

void* copy(void* dst, const void* src, size_t size) noexcept {
  return mempcpy(dst, src, size);
}

void fence() noexcept {
}

#endif

} // namespace cacheless
} // namespace cbu
