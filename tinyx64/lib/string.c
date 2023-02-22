#include "tinyx64.h"

#ifdef __x86_64__
#include <x86intrin.h>
#endif

// Make GCC think the assembler sequence is longer, so that it tends to use its own builtin version instead
#define DUMMY_ASM "\n\
;;;;;;;;;;;\n\
"

void* memset(void* dst, int ch, size_t n) {
#ifdef __x86_64__
  asm volatile("rep stosb" DUMMY_ASM
               : "+D"(dst), "+c"(n)
               : "a"(ch)
               : "memory", "cc");
#else
  char* p = (char*)dst;
  for (; n; --n) *p++ = ch;
#endif
  return dst;
}

void* memcpy(void* dst, const void* src, size_t n) {
  mempcpy(dst, src, n);
  return dst;
}

void* mempcpy(void* dst, const void* src, size_t n) {
#ifdef __x86_64__
  asm volatile("rep movsb" DUMMY_ASM : "+D"(dst), "+S"(src), "+c"(n)::"memory");
#else
  char* d = (char*)dst;
  const char* s = (const char*)src;
  for (; n; --n) *d++ = *s++;
  dst = d;
#endif
  return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
  if ((size_t)(dst - src) < n /*dst >= src && dst < src + n*/) {
    void* ret = dst;
#ifdef __x86_64__
    dst = (void*)((uintptr_t)dst + n - 1);
    src = (void*)((uintptr_t)src + n - 1);
    asm volatile("std; rep movsb; cld" DUMMY_ASM
                 : "+D"(dst), "+S"(src), "+c"(n)::"memory");
#else
    char* d = (char*)dst;
    const char* s = (const char*)src;
    for (; n; --n) d[n - 1] = s[n - 1];
#endif
    return ret;
  } else {
    return memcpy(dst, src, n);
  }
}

size_t strlen(const char* s) {
#if defined __AVX2__
  __v32qi ref = {};
  unsigned filter_bytes = (uintptr_t)s & 31;
  const __v32qi* p = (const __v32qi*)((uintptr_t)s & ~31);
  __v32qi cmp = (*p++ == ref);
  unsigned mask = 0xffffffffu << filter_bytes;
  mask &= __builtin_ia32_pmovmskb256(cmp);
  while (mask == 0) {
    cmp = (ref == *p++);
    mask = __builtin_ia32_pmovmskb256(cmp);
  }
  return ((char*)p - s - 32 + __builtin_ctz(mask));
#elif defined __SSE2__
  __v16qi ref = {};
  unsigned filter_bytes = (uintptr_t)s & 15;
  const __v16qi* p = (const __v16qi*)((uintptr_t)s & ~15);
  __v16qi cmp = (*p++ == ref);
  unsigned mask = 0xffffffffu << filter_bytes;
  mask &= __builtin_ia32_pmovmskb128(cmp);
  while (mask == 0) {
    cmp = ref = (ref == *p++);
    mask = __builtin_ia32_pmovmskb128(cmp);
  }
  return ((char*)p - s - 16 + __builtin_ctz(mask));
#endif

  size_t r = 0;
  while (*s++) ++r;
  return r;
}

int memcmp(const void* a, const void* b, size_t n) {
  const char* p = a;
  const char* q = b;
  for (; n; ++p, ++q, --n) {
    if (*p != *q) return (int)(unsigned char)*p - (int)(unsigned char)*q;
  }
  return 0;
}

void* memchr(const void* s, int c, size_t n) {
  const char* p = s;
  for (; n; ++p, --n) {
    if (*p == (char)c) return (void*)p;
  }
  return NULL;
}

void* memrchr(const void* s, int c, size_t n) {
  const char* p = s + n - 1;
  for (; n; --p, --n) {
    if (*p == (char)c) return (void*)p;
  }
  return NULL;
}

void* rawmemchr(const void* s, int c) {
  const char* p = s;
  while (*p != (char)c) ++p;
  return (void*)p;
}

char* strchr(const char* s, int c) {
  for (const char* p = s; *p; ++p) {
    if (*p == (char)c) return (char*)p;
  }
  return NULL;
}

char* strchrnul(const char* s, int c) {
  const char* p = s;
  while (*p && *p != (char)c) ++p;
  return (char*)p;
}

char *strrchr(const char *s, int c) {
  char *r = NULL;
  for (char *p = (char *)s; *p; ++p) {
    if (*p == (char)c) {
      r = p;
    }
  }
  return r;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    ++a;
    ++b;
  }
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
  while (n > 0 && *a && *b) {
    if (*a != *b)
      return (int)(unsigned char)*a - (int)(unsigned char)*b;
    ++a;
    ++b;
    --n;
  }
  if (n == 0)
    return 0;
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char* utoa10(unsigned v, char* str) {
  if (v == 0) {
    *str++ = '0';
    return str;
  }

  unsigned digits = fast_ilog10(v) + 1;
  for (unsigned i = digits; i; --i) {
    str[i - 1] = '0' + (v % 10);
    v /= 10;
  }

  return (str + digits);
}

char* itoa10(int value, char* str) {
  if (value < 0) {
    value = -value;
    *str++ = '-';
  }
  return utoa10(value, str);
}

Strtou strtou_ex(const char* s) {
  unsigned r = 0;
  while (isdigit(*s)) r = r * 10 + (*s++ - '0');
  return (Strtou){r, (char*)s};
}

unsigned strtou(const char* s, char** endptr) {
  Strtou r = strtou_ex(s);
  if (endptr) *endptr = r.endptr;
  return r.val;
}

char *basename(const char *s) {
  char *slash = strrchr(s, '/');
  if (slash) {
    return slash + 1;
  } else {
    return (char *)s;
  }
}
