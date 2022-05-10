#include "tinyx32.h"

const uint32_t g_fast_ipow10[10] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

static inline unsigned bsr(unsigned x) {
#if __has_builtin(__builtin_ia32_bsrsi)
  return __builtin_ia32_bsrsi(x);
#else
  return __builtin_clz(x) ^ (8 * sizeof(x) - 1);
#endif
}

// Adapted from Hacker's Delight
unsigned fast_ilog10(uint32_t x) {
  unsigned y = (unsigned)(9 * bsr(x)) >> 5;
  if (x >= g_fast_ipow10[y + 1ul]) ++y;
  return y;
}
