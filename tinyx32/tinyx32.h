#pragma once

#if !defined __GNUC__ || !defined __x86_64__ || !defined __ILP32__ || !defined __linux__
# error "Only \"gcc -mx32\" is supported."
#endif

#if defined __PIC__ || defined __PIE__
# error "PIC or PIE must be disabled."
#endif

#if __STDC_HOSTED__
# error "-ffreestanding is reuqired"
#endif

#include "fsyscall.h"

#if ! FSYSCALL_USE
# error "fsyscall must be enabled"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86intrin.h>

#ifdef __cplusplus
# define TX32_INLINE inline
#else
# define TX32_INLINE static inline
#endif

#if defined __cplusplus && __cplusplus >= 201103
# define TX32_CONSTEXPR constexpr
#else
# define TX32_CONSTEXPR
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Functions in start.S
void exit(int) __attribute__((__noreturn__));
void _exit(int) __attribute__((__noreturn__));

// stdlib.c
void abort(void) __attribute__((__cold__, __noreturn__));

// malloc.c
void *malloc(size_t) __attribute__((__malloc__, __warn_unused_result__));
void *calloc(size_t, size_t) __attribute__((__malloc__, __warn_unused_result__));
void *realloc(void *, size_t) __attribute__((__malloc__, __warn_unused_result__));
void *aligned_alloc(size_t, size_t) __attribute__((__malloc__, __warn_unused_result__, __alloc_size__(2)));
void free(void *);

// string.c
void *memset(void *dst, int ch, size_t n) __attribute__((__nonnull__(1)));
void *memcpy(void *dst, const void *src, size_t n) __attribute__((__nonnull__(1, 2)));
void *mempcpy(void *dst, const void *src, size_t n) __attribute__((__nonnull__(1, 2)));
void *memmove(void *dst, const void *src, size_t n) __attribute__((__nonnull__(1, 2)));
size_t strlen(const char *s) __attribute__((__pure__, __nonnull__(1)));
int memcmp(const void *, const void *, size_t) __attribute__((__pure__, __nonnull__(1, 2)));
void *memchr(const void *, int, size_t) __attribute__((__pure__, __nonnull__(1)));
void *memrchr(const void *, int, size_t) __attribute__((__pure__, __nonnull__(1)));
void *rawmemchr(const void *, int) __attribute__((__pure__, __nonnull__(1)));
char *strchr(const char *, int) __attribute__((__pure__, __nonnull__(1)));
char *strchrnul(const char *, int) __attribute__((__pure__, __nonnull__(1)));
int strcmp(const char *, const char *) __attribute__((__pure__, __nonnull__(1, 2)));
int strncmp(const char *, const char *, size_t) __attribute__((__pure__, __nonnull__(1, 2)));

// Non-standard string.c
char *utoa10(unsigned value, char *str);
char *itoa10(int value, char *str);

// unistd.c
int execvpe(const char *exe, char *const *args, char *const *envp);

// Macros
#define STR_LEN(s) (s), sizeof(s) - 1

// Define additional types for SSE/AVX
#ifdef __SSE2__
typedef signed char __v16qs __attribute__((__vector_size__(16)));
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef unsigned __v4su __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
#endif
#ifdef __AVX2__
typedef signed char __v32qs __attribute__((__vector_size__(32)));
typedef unsigned char __v32qu __attribute__((__vector_size__(32)));
typedef unsigned short __v16hu __attribute__((__vector_size__(32)));
typedef unsigned __v8su __attribute__((__vector_size__(32)));
typedef unsigned long long __v4du __attribute__((__vector_size__(32)));
#endif

// Fix for -funsinged-char (GCC only. Clang has no problem)
#if defined __GNUC__ && !defined __clang__

#ifdef __SSE2__
#undef _mm_cmpgt_epi8
#undef _mm_cmplt_epi8
#define _mm_cmpgt_epi8 __mm_cmpgt_epi8_fix
#define _mm_cmplt_epi8 __mm_cmplt_epi8_fix
TX32_INLINE TX32_CONSTEXPR __m128i _mm_cmpgt_epi8(__m128i a, __m128i b) {
	return (__m128i)((__v16qs)a > (__v16qs)b);
}

TX32_INLINE TX32_CONSTEXPR __m128i _mm_cmplt_epi8(__m128i a, __m128i b) {
	return (__m128i)((__v16qs)a < (__v16qs)b);
}
#endif

#ifdef __AVX2__
#undef _mm256_cmpgt_epi8
#define _mm256_cmpgt_epi8 __mm256_cmpgt_epi8_fix
TX32_INLINE TX32_CONSTEXPR __m256i _mm256_cmpgt_epi8(__m256i a, __m256i b) {
	return (__m256i)((__v32qs)a > (__v32qs)b);
}
#endif

#endif // __GNUC__ && !__clang__

#ifdef __cplusplus
} // extern "C"
#endif
