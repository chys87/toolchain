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

#include <stddef.h>

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif
#ifdef __ARM_NEON
# include <arm_neon.h>
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
      return __m128i((__v16qu(v) >= static_cast<unsigned char>(lo)) &
                     (__v16qu(v) <= static_cast<unsigned char>(hi)));
    else
      return __m128i((__v16qu(v) >= static_cast<unsigned char>(lo)) |
                     (__v16qu(v) <= static_cast<unsigned char>(hi)));
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
      return __m256i((__v32qu(v) >= static_cast<unsigned char>(lo)) &
                     (__v32qu(v) <= static_cast<unsigned char>(hi)));
    else
      return __m256i((__v32qu(v) >= static_cast<unsigned char>(lo)) |
                     (__v32qu(v) <= static_cast<unsigned char>(hi)));
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
    return __m512i((__v64qu(v) >= static_cast<unsigned char>(lo)) &
                   (__v64qu(v) <= static_cast<unsigned char>(hi)));
  else
    return __m512i((__v64qu(v) >= static_cast<unsigned char>(lo)) |
                   (__v64qu(v) <= static_cast<unsigned char>(hi)));
}
#endif

namespace arch_detail {

// We no longer try to combine these arrays - each of them should be within one
// single cache line

#define CBU_ARCH_REP2(...) __VA_ARGS__, __VA_ARGS__
#define CBU_ARCH_REP4(...) CBU_ARCH_REP2(CBU_ARCH_REP2(__VA_ARGS__))
#define CBU_ARCH_REP16(...) CBU_ARCH_REP4(CBU_ARCH_REP4(__VA_ARGS__))
#define CBU_ARCH_REP32(...) CBU_ARCH_REP16(CBU_ARCH_REP2(__VA_ARGS__))
alignas(64) inline constexpr char arch_mask_32_ff_32_0[64]{
    CBU_ARCH_REP32('\xff'), CBU_ARCH_REP32(0)};
alignas(64) inline constexpr char arch_mask_32_0_32_ff[64]{
    CBU_ARCH_REP32(0), CBU_ARCH_REP32('\xff')};
#undef CBU_ARCH_REP32
#undef CBU_ARCH_REP16
#undef CBU_ARCH_REP4
#undef CBU_ARCH_REP2
inline constexpr const char* arch_mask_16_ff_16_0 = arch_mask_32_ff_32_0 + 16;
inline constexpr const char* arch_mask_16_0_16_ff = arch_mask_32_0_32_ff + 16;

template <int mul>
  requires(mul == 1 or mul == 2 or mul == 4 or mul == 8)
constexpr int mul_to_shift = (mul == 1)   ? 0
                             : (mul == 2) ? 1
                             : (mul == 4) ? 2
                             : (mul == 8) ? 3
                                          : -1;

}  // namespace arch_detail

#ifdef __SSE__
inline __m128 _mm_lofloat_mask(unsigned num) noexcept {
  // mask + (4 - num) * 4
  return *reinterpret_cast<const __m128_u*>(arch_detail::arch_mask_16_ff_16_0 +
                                            (7 ^ num) * 4l - 12);
}

inline __m128 _mm_lofloat_negmask(unsigned num) noexcept {
  // mask + (4 - num) * 4
  return *reinterpret_cast<const __m128_u*>(arch_detail::arch_mask_16_0_16_ff +
                                            (7 ^ num) * 4l - 12);
}

template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline __m128i _mm_lobyte_mask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  // mask + 16 - n * mul
  // mask + (31 ^ n) - 15   (mul == 1)
  // mask + (15 ^ n) - 14   (mul == 2)
  // mask + (7 ^ n) - 12    (mul == 4)
  // (assuming >> and << has higher precedence)
  return *(const __m128i_u*)(arch_detail::arch_mask_16_ff_16_0 +
                             ((31 >> s) ^ n) * long(mul) - (16 - mul));
}

template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline __m128i _mm_lobyte_negmask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  return *(const __m128i_u*)(arch_detail::arch_mask_16_0_16_ff +
                             ((31 >> s) ^ n) * long(mul) - (16 - mul));
}
#endif

#if defined __SSSE3__
// Code inspired by sysdeps/x86_64/multiarch/varshift.[hc] in glibc
inline __m128i _mm_rshift_byte_epi8(__m128i value, size_t bytes) noexcept {
  alignas(32) static const signed char mask[32] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
  return _mm_shuffle_epi8(value, *(const __m128i_u*)(mask + bytes));
}

inline __m128i load_unaligned_si128(const char* s, size_t length) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  return _mm_maskz_loadu_epi8((1u << length) - 1, s);
#else
  using uintptr_t = unsigned long;
  static_assert(sizeof(uintptr_t) == sizeof(void*), "");

  unsigned us = unsigned(uintptr_t(s));
  unsigned ul = unsigned(length);

  if (((us + ul - 1) ^ (us + 15)) & 4096) {
    // Only do this if movdqu would cross page boundary, but the real range do
    // not.
    __m128i v = *(const __m128i*)(uintptr_t(s) & ~15);
    return _mm_rshift_byte_epi8(v, uintptr_t(s) & 15);
  } else {
    return *(const __m128i_u*)s;
  }
#endif
}

// Guarantee extra bytes are zero
inline __m128i loadz_unaligned_si128(const char* s, size_t length) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  return _mm_maskz_loadu_epi8((1u << length) - 1, s);
#else
  return load_unaligned_si128(s, length) & _mm_lobyte_mask(length);
#endif
}

inline __m128i load_unaligned_si128_str(const char* s) noexcept {
  using uintptr_t = unsigned long;
  unsigned page_offset = uintptr_t(s) % 4096;
  if (page_offset > 4080) {
    unsigned misalign = page_offset - 4080;
    __v16qi v = *(const __v16qi*)(uintptr_t(s) & ~15);
    unsigned mask = _mm_movemask_epi8(__m128i(v == __v16qi()));
    if (mask >> misalign)  // Null terminator found.
      return _mm_rshift_byte_epi8(__m128i(v), misalign);
  }

  return *(const __m128i_u*)s;
}

#endif

#if defined __AVX__
inline __m256 _mm256_lofloat_mask(unsigned num) noexcept {
  // mask + (8 - num) * 4
  return *reinterpret_cast<const __m256_u*>(arch_detail::arch_mask_32_ff_32_0 +
                                            (15 ^ num) * 4l - 28);
}

inline __m256 _mm256_lofloat_negmask(unsigned num) noexcept {
  // mask + (8 - num) * 4
  return *reinterpret_cast<const __m256_u*>(arch_detail::arch_mask_32_0_32_ff +
                                            (15 ^ num) * 4l - 28);
}

template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline __m256i _mm256_lobyte_mask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  return *(const __m256i_u*)(arch_detail::arch_mask_32_ff_32_0 +
                             ((63 >> s) ^ n) * long(mul) - (32 - mul));
}

template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline __m256i _mm256_lobyte_negmask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  return *(const __m256i_u*)(arch_detail::arch_mask_32_0_32_ff +
                             ((63 >> s) ^ n) * long(mul) - (32 - mul));
}
#endif

#if defined __ARM_NEON && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline uint8x16_t neon_lobyte_mask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  // mask + 16 - n * mul
  // mask + (31 ^ n) - 15   (mul == 1)
  // mask + (15 ^ n) - 14   (mul == 2)
  // mask + (7 ^ n) - 12    (mul == 4)
  // (assuming >> and << has higher precedence)
  return *(const uint8x16_t*)(arch_detail::arch_mask_16_ff_16_0 +
                              ((31 >> s) ^ n) * long(mul) - (16 - mul));
}

template <unsigned mul = 1>
  requires(arch_detail::mul_to_shift<mul> >= 0)
inline uint8x16_t neon_lobyte_negmask(unsigned n) noexcept {
  int s = arch_detail::mul_to_shift<mul>;
  return *(const uint8x16_t*)(arch_detail::arch_mask_16_0_16_ff +
                              ((31 >> s) ^ n) * long(mul) - (16 - mul));
}

// Code inspired by sysdeps/x86_64/multiarch/varshift.[hc] in glibc
inline uint8x16_t neon_rshift_u8(uint8x16_t value, size_t bytes) noexcept {
  alignas(32) static const signed char mask[32] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
  return vqtbl1q_u8(value, *(const uint8x16_t*)(mask + bytes));
}

inline uint8x16_t neon_load_unaligned_u8(const char* s,
                                         size_t length) noexcept {
  using uintptr_t = unsigned long;
  static_assert(sizeof(uintptr_t) == sizeof(void*), "");

  unsigned us = unsigned(uintptr_t(s));
  unsigned ul = unsigned(length);

  if (((us + ul - 1) ^ (us + 15)) & 4096) {
    // Only do this if movdqu would cross page boundary, but the real range do
    // not.
    uint8x16_t v = *(const uint8x16_t*)(uintptr_t(s) & ~15);
    return neon_rshift_u8(v, uintptr_t(s) & 15);
  } else {
    return *(const uint8x16_t*)s;
  }
}
#endif

#if defined __AVX2__
inline __m256i compose_si256(__m128i lo, __m128i hi) noexcept {
  return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

inline __m256i load_unaligned_si256(const char* s, size_t length) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  // If there's a simple way to for SHLX instead of SHL, we can use 32-bit
  // instead of 64-bit mask.
  return _mm256_maskz_loadu_epi8((1ull << length) - 1, s);
#else
  using uintptr_t = unsigned long;
  static_assert(sizeof(uintptr_t) == sizeof(void*), "");

  unsigned us = unsigned(uintptr_t(s));
  unsigned ul = unsigned(length);

  if (((us + ul - 1) ^ (us + 31)) & 4096) {
    // Only do this if movdqu would cross page boundary, but the real range do
    // not.
    __m128i hi = _mm_rshift_byte_epi8(*(const __m128i_u*)(s + length - 16),
                                      (32 - length) & 15);
    if (length <= 16) {
      return _mm256_castsi128_si256(hi);
    } else {
      __m128i lo = *(const __m128i_u*)s;
      return compose_si256(lo, hi);
    }
  } else {
    return *(const __m256i_u*)s;
  }
#endif
}

// Guarantee extra bytes are zero
inline __m256i loadz_unaligned_si256(const char* s, size_t length) noexcept {
#if defined __AVX512VL__ && defined __AVX512BW__
  // If there's a simple way to for SHLX instead of SHL, we can use 32-bit
  // instead of 64-bit mask.
  return _mm256_maskz_loadu_epi8((1ull << length) - 1, s);
#else
  return load_unaligned_si256(s, length) & _mm256_lobyte_mask(length);
#endif
}

#endif

}  // namespace cbu
