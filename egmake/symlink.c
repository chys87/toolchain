/*
egmake, Enhanced GNU make
Copyright (C) 2014-2024, chys <admin@CHYS.INFO>

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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsyscall.h"
#include "util.h"

static int do_symlink(const char* target, const char* link) {
  int rc = fsys_symlink(target, link);
  if (rc != 0 && fsys_errno(rc, EEXIST)) {
    char buf[PATH_MAX];
    ssize_t l = fsys_readlink(link, buf, sizeof(buf));
    if (strncmp(target, buf, l) == 0 && target[l] == '\0') return 0;
    fsys_unlink(link);
    rc = fsys_symlink(target, link);
  }
  return rc;
}

char* func_symlinkrel(const char* nm, unsigned int argc, char** argv) {
  if (argc != 2) return NULL;
  int rc;
  char* orig_target = argv[0];
  const char* link = argv[1];
  const char* slash = strrchr(link, '/');
  char* target = orig_target;
  if (slash == NULL) {
    target = egmake_relpath(target, ".");
  } else {
    size_t len = slash - link;
    char* base_dir = malloc(len + 1);
    if (base_dir == NULL) goto error;
    memcpy(base_dir, link, len);
    base_dir[len] = '\0';
    target = egmake_relpath(target, base_dir);
    free(base_dir);
  }
  if (target == NULL) goto error;
  rc = do_symlink(target, link);
  free(target);
  if (rc != 0) goto error;
  return NULL;

error:
  fprintf(stderr, "Failed to symlink %s <- %s\n", orig_target, link);
  return NULL;
}

char* func_symlink(const char* nm, unsigned int argc, char** argv) {
  if (argc == 2) {
    if (do_symlink(argv[0], argv[1]) != 0) {
      fprintf(stderr, "Failed to symlink %s <- %s\n", argv[0], argv[1]);
    }
  }
  return NULL;
}

/*
MAKEFILE-TEST-BEGIN

$(shell rm -rf /tmp/egmake-link*)
$(EGM.symlink /etc/fstab,/tmp/egmake-link1)
$(EGM.symlinkrel /etc/mtab,/tmp/egmake-link2)
$(EGM.symlinkrel /etc/fstab,/tmp/egmake-link2)

test:
	test "$(EGM.readlink /tmp/egmake-link1)" = "/etc/fstab"
	test "$(EGM.readlink /tmp/egmake-link2)" = "../etc/fstab"

MAKEFILE-TEST-END
*/
