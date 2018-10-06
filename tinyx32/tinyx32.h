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
int strcmp(const char *, const char *) __attribute__((__pure__, __nonnull__(1, 2)));

// Non-standard string.c
char *utoa10(unsigned value, char *str);
char *itoa10(int value, char *str);

// Macros
#define STR_LEN(s) (s), sizeof(s) - 1

#ifdef __cplusplus
} // extern "C"
#endif
