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

#include <gnumake.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fsyscall.h"
#include "util.h"

typedef struct PathPart {
  const char* p;
  size_t l;
} PathPart;

static bool part_equal(const PathPart* a, const PathPart* b) {
  return (a->l == b->l && memcmp(a->p, b->p, a->l) == 0);
}

typedef struct AbsPath {
  char* allocated;  // String pool.  May be NULL if original path is absolute
  PathPart* parts;
  unsigned part_count;
  unsigned max_len;  // upper limit of concatenated absolute path
} AbsPath;

static void abspath_free(AbsPath* ap) {
  free(ap->allocated);
  free(ap->parts);
}

static bool abspath_get(const char* path, AbsPath* res) {
  *res = (AbsPath){};
  size_t path_len = strlen(path);
  if (path_len == 0) return false;
  if (*path != '/') {
    // Get absolute path
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return false;
    size_t cwd_l = strlen(cwd);
    char* new_p = malloc(cwd_l + 1 + path_len + 1);
    if (new_p == NULL) return false;
    res->allocated = new_p;
    *Mempcpy(Mempcpy(Mempcpy(new_p, cwd, cwd_l), "/", 1), path, path_len) =
        '\0';
    path_len += cwd_l + 1;
    path = new_p;
  }
  res->max_len = path_len;
  PathPart* parts = res->parts = (PathPart*)malloc(path_len * sizeof(PathPart));
  if (parts == NULL) {
    free(res->allocated);
    return false;
  }

  size_t part_count = 0;
  const char* e = path + path_len;
  while (path < e) {
    while (path < e && *path == '/') ++path;
    if (path >= e) break;
    const char* next_slash = (char*)memchr(path, '/', e - path);
    if (next_slash == NULL) next_slash = e;
    size_t part_l = next_slash - path;
    if (part_l == 1 && *path == '.') {
    } else if (part_l == 2 && memcmp(path, "..", 2) == 0) {
      if (part_count) --part_count;
    } else {
      parts[part_count++] = (PathPart){path, part_l};
    }
    path = next_slash;
  }
  res->part_count = part_count;
  return true;
}


char *func_relpath(const char *nm, unsigned int argc, char **argv) {
  if (argc != 2) return NULL;

  AbsPath path_abs;
  if (!abspath_get(argv[0], &path_abs)) return NULL;

  AbsPath start_abs;
  if (!abspath_get(argv[1], &start_abs)) {
    abspath_free(&path_abs);
    return NULL;
  }

  size_t k = 0;
  while (k < path_abs.part_count && k < start_abs.part_count &&
         part_equal(&path_abs.parts[k], &start_abs.parts[k]))
    ++k;

  unsigned uppers = start_abs.part_count - k;

  char* res = gmk_alloc(path_abs.max_len + uppers * 3 + 16);
  if (res != NULL)  {
    char* w = res;
    for (unsigned i = uppers; i; --i) w = Mempcpy(w, "../", 3);
    for (unsigned n = path_abs.part_count; k < n; ++k) {
      w = Mempcpy(w, path_abs.parts[k].p, path_abs.parts[k].l);
      *w++ = '/';
    }
    if (w == res) {
      strcpy(res, ".");
    } else {
      --w;
      *w = '\0';
    }
  }

  abspath_free(&start_abs);
  abspath_free(&path_abs);
  return res;
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.relpath /etc,/)" = "etc"
	test "$(EGM.relpath /,/etc)" = ".."
	test "$(EGM.relpath /,/)" = "."
	test "$(EGM.relpath /a/b/c,/a/e/d)" = "../../b/c"
	test "$(EGM.relpath .,abc)" = ".."
	test "$(EGM.relpath xyz,abc)" = "../xyz"
	test "$(EGM.relpath xyz,.)" = "xyz"

MAKEFILE-TEST-END
*/
