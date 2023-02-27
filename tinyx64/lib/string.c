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
#elif __has_builtin(__builtin_memset_inline)
  char* p = (char*)dst;
  if (n > 32) {
    while (n > 32) {
      __builtin_memset_inline(p, ch, 32);
      p += 32;
      n -= 32;
    }
    __builtin_memset_inline(p + n - 32, ch, 32);
    return dst;
  }

  uint64_t vv = (uint8_t)ch * 0x0101010101010101ull;

  if (n > 8) {
    if (n >= 16) {
      *(uint64_t*)(p + 8) = vv;
      *(uint64_t*)p = vv;
      *(uint64_t*)(p + n - 8) = vv;
      *(uint64_t*)(p + n - 16) = vv;
    } else {
      *(uint64_t*)p = vv;
      *(uint64_t*)(p + n - 8) = vv;
    }
  } else if (n >= 4) {
    *(uint32_t*)p = vv;
    *(uint32_t*)(p + n - 4) = vv;
  } else if (n) {
    p[0] = ch;
    p[n / 2] = ch;
    p[n - 1] = ch;
  }
#else
  char* p = (char*)dst;
  for (; n; --n) *p++ = ch;
#endif
  return dst;
}

static void* mempcpy_impl(void* dst, const void* src, size_t n) {
#ifdef __x86_64__
  asm volatile("rep movsb" DUMMY_ASM : "+D"(dst), "+S"(src), "+c"(n)::"memory");
#elif defined __ARM_NEON && __has_builtin(__builtin_memcpy_inline)
  char* d = (char*)dst;
  const char* s = (const char*)src;
  dst = d + n;
  if (n > 32) {
    while (n > 32) {
      __builtin_memcpy_inline(d, s, 32);
      d += 32;
      s += 32;
      n -= 32;
    }
    __builtin_memcpy_inline(d + n - 32, s + n - 32, 32);
  } else if (n > 8) {
    if (n > 16) {
      uint64x2_t a = *(const uint64x2_t*)s;
      uint64x2_t b = *(const uint64x2_t*)(s + n - 16);
      *(uint64x2_t*)d = a;
      *(uint64x2_t*)(d + n - 16) = b;
    } else {
      uint64_t a = *(const uint64_t*)s;
      uint64_t b = *(const uint64_t*)(s + n - 8);
      *(uint64_t*)d = a;
      *(uint64_t*)(d + n - 8) = b;
    }
  } else if (n >= 4) {
    uint32_t a = *(const uint32_t*)s;
    uint32_t b = *(const uint32_t*)(s + n - 4);
    *(uint32_t*)d = a;
    *(uint32_t*)(d + n - 4) = b;
  } else if (n) {
    // n is 0, 1 or 2
    uint8_t a = s[0];
    uint8_t b = s[n / 2];
    uint8_t c = s[n - 1];
    d[0] = a;
    d[n / 2] = b;
    d[n - 1] = c;
  }
#else
  char* d = (char*)dst;
  const char* s = (const char*)src;
  for (; n; --n) *d++ = *s++;
  dst = d;
#endif
  return dst;
}

static void* memcpy_impl(void* dst, const void* src, size_t n) {
  mempcpy_impl(dst, src, n);
  return dst;
}

void* memcpy(void* dst, const void* src, size_t n) {
  return memcpy_impl(dst, src, n);
}

void* mempcpy(void* dst, const void* src, size_t n) {
  return mempcpy_impl(dst, src, n);
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
    return memcpy_impl(dst, src, n);
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
#elif defined __ARM_NEON
  uint8x16_t ref = {};
  unsigned filter_bytes = (uintptr_t)s & 15;
  const uint8x16_t* p = (const uint8x16_t*)((uintptr_t)s & ~15);
  uint8x8_t cmp = vshrn_n_u16(vreinterpretq_u16_u8(*p == ref), 4);
  uint64_t mask = ((uint64_t)(-1) << (filter_bytes * 4)) &
                  vget_lane_u64(vreinterpret_u64_u8(cmp), 0);
  while (mask == 0) {
    cmp = vshrn_n_u16(vreinterpretq_u16_u8(ref == *++p), 4);
    mask = vget_lane_u64(vreinterpret_u64_u8(cmp), 0);
  }
  return ((char*)p - s + __builtin_ctzll(mask) / 4);
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

int bcmp(const void* a, const void* b, size_t n) {
  const char* p = a;
  const char* q = b;
  if (n > 8) {
    while (n > 8) {
      if (*(const uint64_t*)p != *(const uint64_t*)q) return 1;
      p += 8;
      q += 8;
      n -= 8;
    }
    return *(const uint64_t*)(p + n - 8) != *(const uint64_t*)(q + n - 8);
  } else if (n >= 4) {
    return *(const uint32_t*)p != *(const uint32_t*)q ||
           *(const uint32_t*)(p + n - 4) != *(const uint32_t*)(q + n - 4);
  } else if (n) {
    return p[0] != q[0] || p[n / 2] != q[n / 2] || p[n - 1] != q[n - 1];
  } else {
    return 0;
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

char* strrchr(const char* s, int c) { return strrchr_ex(s, c).ptr; }

StrRChrEx strrchr_ex(const char* s, int c) {
  char* r = NULL;
  char* p = (char*)s;
  for (; *p; ++p) {
    if (*p == (char)c) {
      r = p;
    }
  }
  return (StrRChrEx){r, p};
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

StrRChrEx basename_ex(const char *s) {
  StrRChrEx res = strrchr_ex(s, '/');
  if (res.ptr) {
    ++res.ptr;
  } else {
    res.ptr = (char*)s;
  }
  return res;
}
