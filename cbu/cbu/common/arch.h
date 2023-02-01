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

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif

namespace cbu {

#ifdef __SSE2__
// [lo, hi] (lo <= hi) or [-128,hi] union [lo,127] (lo > hi)
inline __m128i in_range_epi8(__m128i v, signed char lo,
                             signed char hi) noexcept {
  if (static_cast<signed char>(hi + 1) == lo) {
    return _mm_set1_epi8(-1);
  } else if (lo == -128) {
    return _mm_cmpgt_epi8(_mm_set1_epi8(hi + 1), v);
  } else if (hi == 127) {
    return _mm_cmpgt_epi8(v, _mm_set1_epi8(lo - 1));
  } else if (lo == hi) {
    return _mm_cmpeq_epi8(v, _mm_set1_epi8(lo));
  } else {
#if defined __AVX512VL__ && defined __AVX512BW__
    // With AVX-512VL and AVX-512BW, gcc/clang generates satisfactory code
    if (static_cast<unsigned char>(lo) <= static_cast<unsigned char>(hi))
      return (__v16qu(v) >= static_cast<unsigned char>(lo)) &
             (__v16qu(v) <= static_cast<unsigned char>(hi));
    else
      return (__v16qu(v) >= static_cast<unsigned char>(lo)) |
             (__v16qu(v) <= static_cast<unsigned char>(hi));
#endif
    v = _mm_add_epi8(v, _mm_set1_epi8(127 - hi));
    return _mm_cmpgt_epi8(v, _mm_set1_epi8(127 - (hi - lo) - 1));
  }
}
#endif

#ifdef __AVX2__
// [lo, hi] (lo <= hi) or [-128,hi] union [lo,127] (lo > hi)
inline __m256i in_range_epi8(__m256i v, signed char lo,
                             signed char hi) noexcept {
  if (static_cast<signed char>(hi + 1) == lo) {
    return _mm256_set1_epi8(-1);
  } else if (lo == -128) {
    return _mm256_cmpgt_epi8(_mm256_set1_epi8(hi + 1), v);
  } else if (hi == 127) {
    return _mm256_cmpgt_epi8(v, _mm256_set1_epi8(lo - 1));
  } else if (lo == hi) {
    return _mm256_cmpeq_epi8(v, _mm256_set1_epi8(lo));
  } else {
#if defined __AVX512VL__ && defined __AVX512BW__
    // With AVX-512VL and AVX-512BW, gcc/clang generates satisfactory code
    if (static_cast<unsigned char>(lo) <= static_cast<unsigned char>(hi))
      return (__v32qu(v) >= static_cast<unsigned char>(lo)) &
             (__v32qu(v) <= static_cast<unsigned char>(hi));
    else
      return (__v32qu(v) >= static_cast<unsigned char>(lo)) |
             (__v32qu(v) <= static_cast<unsigned char>(hi));
#endif
    v = _mm256_add_epi8(v, _mm256_set1_epi8(127 - hi));
    return _mm256_cmpgt_epi8(v, _mm256_set1_epi8(127 - (hi - lo) - 1));
  }
}
#endif

#if defined __AVX512BW__
inline __m512i in_range_epi8(__m512i v, signed char lo,
                             signed char hi) noexcept {
  if (static_cast<signed char>(hi + 1) == lo)
    return _mm512_set1_epi8(-1);
  else if (static_cast<unsigned char>(lo) <= static_cast<unsigned char>(hi))
    return (__v64qu(v) >= static_cast<unsigned char>(lo)) &
           (__v64qu(v) <= static_cast<unsigned char>(hi));
  else
    return (__v64qu(v) >= static_cast<unsigned char>(lo)) |
           (__v64qu(v) <= static_cast<unsigned char>(hi));
}
#endif

}  // namespace cbu
