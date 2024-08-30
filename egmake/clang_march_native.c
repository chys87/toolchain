/*
egmake, Enhanced GNU make
Copyright (C) 2014-2023, chys <admin@CHYS.INFO>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "fsyscall.h"
#include "util.h"

static inline bool is_alnum(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9');
}

#ifdef __aarch64__
static char* gen_aarch64(char* w) {
  char cpuinfo[4096];
  int fd = fsys_open2("/proc/cpuinfo", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "Failed to open /proc/cpuinfo: %s\n",
            strerrordesc_np(fsys_errno_val(fd)));
    return w;
  }
  size_t L = 0;
  for (;;) {
    ssize_t l = fsys_read(fd, cpuinfo + L, sizeof(cpuinfo) - 1 - L);
    if (l <= 0) {
      fsys_close(fd);
      if (l < 0) {
        fprintf(stderr, "Failed to read /proc/cpuinfo: %s\n",
                strerrordesc_np(fsys_errno_val(l)));
        return w;
      }
      break;
    }
    L += l;
    if (L >= sizeof(cpuinfo) - 1) {
      fsys_close(fd);
      break;
    }
  }
  cpuinfo[L] = 0;
  const char* p = strstr(cpuinfo, "Features");
  if (p == NULL) {
    fprintf(stderr, "Failed to parse /proc/cpuinfo\n");
    return w;
  }
  p += strlen("Features");
  while (*p == ' ' || *p == '\t') ++p;
  if (*p == ':') ++p;

  bool has_sve = false;

  for (;;) {
    while (*p == ' ' || *p == '\t') ++p;
    if (!is_alnum(*p)) break;
    const char* e = p + 1;
    while (is_alnum(*e)) ++e;

#define EQ(str)(e - p == strlen(str) && memcmp(p, str, strlen(str)) == 0)

    if (EQ("fp") || EQ("aes") || EQ("sha2") || EQ("sha3") || EQ("sm4") ||
        EQ("sve") || EQ("flagm") || EQ("ssbs") || EQ("sb") || EQ("sve2") ||
        EQ("i8mm") || EQ("bf16")) {
      if (EQ("sve") || EQ("sve2")) has_sve = true;
      *w++ = '+';
      w = Mempcpy(w, p, e - p);
    } else if (EQ("crc32")) {
      w = Mempcpy(w, "+crc", strlen("+crc"));
    } else if (EQ("atomics")) {
      w = Mempcpy(w, "+lse", strlen("+lse"));
    }
    p = e;
  }

  if (has_sve) {
    // It appears clang actually generates worse code with -msve-vector-bits.
    // With scalable bits, it only uses SVE in loop vectorization, which is
    // good.  With fixed width, it even attempts to use SVE when storing 16 or
    // 32 bits of zeros, resulting in longer code.
#if 0
    int bytes = fsys_prctl_cptr(PR_SVE_GET_VL, NULL) & PR_SVE_VL_LEN_MASK;
    w += snprintf(w, 64, " -msve-vector-bits=%d", bytes * 8);
#endif
  }

  return w;
}
#endif

// Generate "-march=native+***" for clang.
// Clang 16 fails to detect some usable features with "-march=native"
char *func_clang_march_native(const char *nm, unsigned int argc, char **argv) {
#ifdef __aarch64__
  char buffer[1024];
  char* w = Mempcpy(buffer, "-march=native", strlen("-march=native"));
  w = gen_aarch64(w);
  return strdup_to_gmk_with_len(buffer, w - buffer);
#else
  return strdup_to_gmk("-march=native");
#endif
}

/*
MAKEFILE-TEST-BEGIN

test:
	echo "$(EGM.clang_march_native )" | grep -F -- '-march=native'

MAKEFILE-TEST-END
*/
