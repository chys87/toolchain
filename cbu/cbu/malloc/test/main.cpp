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

#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif

// Define a week "cbu_sized_free"
// If we're not linking to cbu_malloc, use standard free
extern "C" {
void cbu_sized_free(void *, size_t) __attribute__((__weak__));
void cbu_sized_free(void *p, size_t) { free(p); }
} // extern "C"

namespace {

thread_local unsigned seed = 0;

// Accelerate rand_r
inline unsigned rand_r(unsigned *seed) {
  return (*seed = 1664525 * *seed + 1013904223);
}

bool check_const(const char *p, char c, size_t len) {
#if defined __AVX2__
  // Try to minimize overhead of check_** functions
  __m256i vx = _mm256_set1_epi8(c);
  while (len && (uintptr_t(p) & 31)) {
    if (*p != c) return false;
    ++p;
    --len;
  }
  __m256i v = _mm256_setzero_si256();
  while (len >= 16 * 32) {
    len -= 16 * 32;
    _mm_prefetch(p + 512, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 1 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 2 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 3 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 4 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 5 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 6 * 64, _MM_HINT_NTA);
    _mm_prefetch(p + 512 + 7 * 64, _MM_HINT_NTA);
    v |= vx ^ *(const __m256i*)p;
    v |= vx ^ *(const __m256i*)(p + 1 * 32);
    v |= vx ^ *(const __m256i*)(p + 2 * 32);
    v |= vx ^ *(const __m256i*)(p + 3 * 32);
    v |= vx ^ *(const __m256i*)(p + 4 * 32);
    v |= vx ^ *(const __m256i*)(p + 5 * 32);
    v |= vx ^ *(const __m256i*)(p + 6 * 32);
    v |= vx ^ *(const __m256i*)(p + 7 * 32);
    v |= vx ^ *(const __m256i*)(p + 8 * 32);
    v |= vx ^ *(const __m256i*)(p + 9 * 32);
    v |= vx ^ *(const __m256i*)(p + 10 * 32);
    v |= vx ^ *(const __m256i*)(p + 11 * 32);
    v |= vx ^ *(const __m256i*)(p + 12 * 32);
    v |= vx ^ *(const __m256i*)(p + 13 * 32);
    v |= vx ^ *(const __m256i*)(p + 14 * 32);
    v |= vx ^ *(const __m256i*)(p + 15 * 32);
    p += 16 * 32;
  }
  while (len >= 32) {
    len -= 32;
    v |= vx ^ *(const __m256i*)p;
    p += 32;
  }
  if (!_mm256_testz_si256(v, v)) return false;
#endif
  for (; len; --len)
    if (*p++ != c)
      return false;
  return true;
}

bool check_all_zeros(const char *p, size_t len) {
  return check_const(p, 0, len);
}

template <size_t N, size_t MAXBLOCK>
void *basic_check(void* = nullptr) {
  char* p[N];
  unsigned l[N];
  for (size_t k=0; k<N; ++k) {
    size_t len = rand_r(&seed) % MAXBLOCK;
    p[k] = (char *)malloc(len);
    l[k] = len;
    memset(p[k], k, len);
  }
  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r(&seed) % N;
    if (p[k] == 0)
      continue;
    if (!check_const(p[k], k, l[k]))
      return (void *)1;
    free (p[k]);
    p[k] = 0;
  }
  for (size_t k=0; k<N; ++k)
    free(p[k]);
  return nullptr;
}

template <size_t N, size_t MAXBLOCK>
void *calloc_check(void* = nullptr) {
  char *p[N];
  unsigned l[N];
  for (size_t k=0; k<N; ++k) {
    size_t len = rand_r (&seed) % MAXBLOCK;
    p[k] = (char *)calloc (len, 1);
    l[k] = len;
    if (!check_all_zeros (p[k], len))
      return (void *)1;
    memset (p[k], k, len);
  }
  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r (&seed) % N;
    if (p[k] == 0)
      continue;
    for (size_t i=0; i<l[k]; ++i)
      if ((unsigned char)p[k][i] != (unsigned char)k)
        return (void *)1;
    free (p[k]);
    p[k] = 0;
  }
  for (size_t k=0; k<N; ++k)
    free (p[k]);
  return nullptr;
}

template <size_t N, size_t MAXBLOCK>
void *realloc_check(void* = 0) {
  char *p[N];
  unsigned l[N];
  for (size_t k=0; k<N; ++k) {
    size_t len = rand_r (&seed) % MAXBLOCK;
    p[k] = (char *)calloc (len, 1);
    if (!check_all_zeros (p[k], len))
      return (void *)1;
    memset (p[k], k, len);
    size_t newlen = rand_r(&seed) % MAXBLOCK;
    p[k] = (char *)realloc (p[k], newlen);
    l[k] = newlen;
    if (newlen > len)
      memset(p[k] + len, k, newlen - len);
  }
  // Likely shrinking
  for (size_t k=0; k<N; ++k) {
    size_t newlen = rand_r (&seed) % (MAXBLOCK / 16);
    size_t oldlen = l[k];
    l[k] = newlen;
    p[k] = (char *)realloc (p[k], newlen);
    if (newlen > oldlen)
      memset (p[k] + oldlen, k, newlen - oldlen);
  }
  // Likely growing
  for (size_t k=0; k<N; ++k) {
    size_t newlen = rand_r (&seed) % MAXBLOCK;
    size_t oldlen = l[k];
    l[k] = newlen;
    p[k] = (char *)realloc (p[k], newlen);
    if (newlen > oldlen)
      memset (p[k] + oldlen, k, newlen - oldlen);
  }
  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r (&seed) % N;
    if (p[k] == 0)
      continue;
    if (!check_const (p[k], k, l[k]))
      return (void *)1;
    free (p[k]);
    p[k] = 0;
  }
  for (size_t k=0; k<N; ++k)
    free(p[k]);
  return nullptr;
}

void* align_check(void* = 0) {
  for (size_t boundary = 8; boundary <= 4096; boundary *= 2) {
    void* p =
        aligned_alloc(boundary, (rand_r(&seed) % 8192 + 1 + boundary - 1) /
                                    boundary * boundary);
    if ((uintptr_t)p % boundary) {
      fprintf (stderr, "Misalign %p on %zu boundary.\n", p, boundary);
      return (void *)1;
    }
    void* q = nullptr;
    // posix_memalign is marked as warn_unused_result
    if (posix_memalign(&q, boundary, rand_r(&seed) % 8192 + 1) == 0) {}
    if ((uintptr_t)q % boundary) {
      fprintf (stderr, "Misalign %p on %zu boundary.\n", q, boundary);
      return (void *)1;
    }
    free(p);
    free(q);
  }
  void* p = valloc(rand_r(&seed) % 8192 + 1);
  if ((uintptr_t)p % 4096)
    return (void *)1;
  free (p);
  return nullptr;
}

double diff(const timespec &a, const timespec &b) {
  return (b.tv_sec - a.tv_sec) + 1.e-9 * (b.tv_nsec - a.tv_nsec);
}

class Perf {
 public:
  Perf() {
    clock_gettime(CLOCK_MONOTONIC, &p_[0]);
  }
  void tick(int k) {
    clock_gettime(CLOCK_MONOTONIC, &p_[k]);
  }

  double v(int k) { return diff(p_[k-1], p_[k]); }

 private:
  struct timespec p_[8];
};

template <bool CALLOC, size_t N, size_t M, bool SIZED_FREE=false>
[[gnu::noinline]]
void performance_test() {
  Perf perf;

  void *p[N];
  size_t sz[N];
  for (size_t k=0; k<N; ++k) {
    size_t siz = rand_r (&seed) % M;
    sz[k] = siz;
    p[k] = CALLOC ? calloc(siz,1) : malloc (siz);
  }

  perf.tick(1);

  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r (&seed) % N;
    if (p[k]) {
      if (SIZED_FREE)
        cbu_sized_free(p[k], sz[k]);
      else
        free(p[k]);
      p[k] = NULL;
      sz[k] = 0;
    }
  }
  for (size_t k=0; k<N; ++k) {
    if (SIZED_FREE)
      cbu_sized_free(p[k], sz[k]);
    else
      free(p[k]);
  }

  perf.tick(2);

  printf(" %12.3g %12.3g\n", perf.v(1), perf.v(2));
}

template <size_t N, size_t MAXBLOCK>
[[gnu::noinline]]
void performance_realloc() {
  Perf perf;

  void *p[N];
  for (size_t k=0; k<N; ++k) {
    size_t siz = rand_r (&seed) % MAXBLOCK;
    p[k] = malloc (siz);
  }

  perf.tick(1);

  // Likely shrinking
  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r (&seed) % N;
    p[k] = realloc(p[k], rand_r(&seed) % (MAXBLOCK / 16));
  }
  // Likely growing
  for (size_t j=0; j<N; ++j) {
    size_t k = rand_r (&seed) % N;
    p[k] = realloc(p[k], rand_r(&seed) % MAXBLOCK);
  }

  perf.tick(2);

  for (size_t k=0; k<N; ++k)
    free (p[k]);

  perf.tick(3);

  printf(" %12.3g %12.3g %12.3g\n", perf.v(1), perf.v(2), perf.v(3));
}

template <size_t N>
[[gnu::noinline]]
void performance_real() {
  Perf perf;

  void *p[N] = {};
  for (size_t k = 0; k < N * 8; ++k) {
    size_t i = rand_r(&seed) % N;
    int rnd = rand_r(&seed);
    if ((rnd & 3) == 0) {
      free(p[i]);
      p[i] = nullptr;
    } else {
      float r = rand_r(&seed) / RAND_MAX;
      r = powf(r, 16);
      size_t sz = unsigned(32 * 1024 * 1024 * r);
      if (p[i])
        p[i] = realloc(p[i], sz);
      else
        p[i] = malloc(sz);
    }
  }

  perf.tick(1);

  printf(" %12.3g\n", perf.v(1));
}

} // namespace

int main (int argc, char **argv) {
  // Some special tests
  if (argv[1] && !strcmp(argv[1], "-1")) {
    size_t m = 64 * 1024 * 1024;
    void *volatile p = malloc(m);
    void *volatile r = malloc(1);
    void *volatile q = malloc(m);
    free(p);
    free(q);
    free(r);
    return 0;
  }

  setlinebuf(stdout);

  struct timespec starttime, endtime;
  for (int i=0; i<3; ++i) {

    puts ("Testing allocating and freeing small blocks...");
    if (basic_check<8192,1024> ()) return 1;
    if (basic_check<8192,1024> ()) return 1;
    puts ("Testing allocating and freeing large blocks...");
    if (basic_check<128,1024*1024> ()) return 1;
    if (basic_check<128,1024*1024> ()) return 1;
    puts ("Testing calloc with small blocks...");
    if (calloc_check<8192,1024> ()) return 1;
    if (calloc_check<8192,1024> ()) return 1;
    puts ("Testing calloc with large blocks...");
    if (calloc_check<1024,1024*1024> ()) return 1;
    puts ("Testing realloc with small blocks...");
    if (realloc_check<8192,1024> ()) return 1;
    puts ("Testing realloc with large blocks...");
    if (realloc_check<128,1024*1024> ()) return 1;
    puts ("Testing alignment...");
    if (align_check ()) return 1;
#if 0
    puts("Testing sized free...");
    if (sized_delete_check()) return 1;
#endif

    puts ("Testing multithreading...");
    pthread_t id[128];

    clock_gettime (CLOCK_MONOTONIC, &starttime);
    size_t tc = 0;
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16384,1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<128,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<128,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<128,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<128,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, basic_check<16,32*1024*1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, calloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, realloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, realloc_check<8192,1024>, 0);
    pthread_create (&id[tc++], NULL, realloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, realloc_check<64,1024*1024>, 0);
    pthread_create (&id[tc++], NULL, align_check, 0);
    pthread_create (&id[tc++], NULL, align_check, 0);
    int err = 0;
    for (size_t i = 0; i < tc; ++i) {
      void *v;
      pthread_join(id[i], &v);
      if (v) {
        fprintf(stderr, "The %d-th thread fails\n", int(i));
        err = 1;
      }
    }
    if (err)
      return 1;

    clock_gettime (CLOCK_MONOTONIC, &endtime);
    printf("multithreaded: %.3gs\n",
           (endtime.tv_sec - starttime.tv_sec) +
               1.e-9 * (endtime.tv_nsec - starttime.tv_nsec));

    puts ("Now testing performance.");
#define TEST(name,args...)	\
    /*seed = 0; */\
    printf("%15s", name); args

    TEST(" 256B malloc:", performance_test<false,131072,256>());
    TEST(" 256B malloc:", performance_test<false, 32768,256>());
    TEST(" 1KiB malloc:", performance_test<false,65536,1024>());
    TEST(" 1KiB malloc:", performance_test<false, 8192,1024>());
    TEST(" 1KiB malloc:", performance_test<false, 4096,1024>());
    TEST("64KiB malloc:", performance_test<false,4096,64*1024>());
    TEST("64KiB szfree:", performance_test<false,4096,64*1024, true>());
    TEST(" 1MiB malloc:", performance_test<false, 128,1024*1024>());
    TEST(" 1MiB szfree:", performance_test<false, 128,1024*1024, true>());
    TEST("32MiB malloc:", performance_test<false, 16,32*1024*1024>());
    TEST("32MiB szfree:", performance_test<false, 16,32*1024*1024, true>());

    TEST(" 256B calloc:", performance_test<true, 131072,256>());
    TEST(" 256B calloc:", performance_test<true, 32768,256>());
    TEST(" 1KiB calloc:", performance_test<true,65536,1024>());
    TEST(" 1KiB calloc:", performance_test<true,16384,1024>());
    TEST(" 1KiB calloc:", performance_test<true, 8192,1024>());
    TEST("64KiB calloc:", performance_test<true, 256,64*1024>());
    TEST(" 1MiB calloc:", performance_test<true, 256,1024*1024>());
    TEST("32MiB calloc:", performance_test<true,16,32*1024*1024>());

    TEST("1KiB realloc:", performance_realloc<65536,1024>());
    TEST("1MiB realloc:", performance_realloc<128,1024*1024>());
    TEST("32MiB realloc:", performance_realloc<16,32*1024*1024>());

    TEST("\"Real\" 128", performance_real<128>());
    TEST("\"Real\" 1024", performance_real<1024>());
    TEST("\"Real\" 2048", performance_real<2048>());
    TEST("\"Real\" 4096", performance_real<4096>());
    TEST("\"Real\" 8192", performance_real<8192>());
  }

  struct rusage ru;
  getrusage (RUSAGE_SELF, &ru);
  printf ("MaxRSS\n%ld\n", ru.ru_maxrss);

  // Don't use system, which frequently triggers overcommit failures

  char cmd[256];
  const char *args[] = {"sh", "-c", cmd, nullptr};

  auto sys = [args]() {
    pid_t pid = vfork();
    if (pid == 0) {
      execv("/bin/sh", (char*const*)args);
      _exit(127);
    }
    waitpid(pid, nullptr, 0);
  };

  snprintf(cmd, sizeof(cmd), "fgrep AnonHuge /proc/%u/smaps | fgrep -v ' 0 k'",
           getpid());
  puts("AnonHuge");
  sys();

  snprintf(cmd, sizeof(cmd), "fgrep VmRSS /proc/%u/status", getpid());
  puts("RSS");
  sys();

  return 0;
}
