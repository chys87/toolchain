/*
egmake, Enhanced GNU make
Copyright (C) 2014-2020, chys <admin@CHYS.INFO>

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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fsyscall.h"

static void mkdirp(char *path) {
  // Fast path
  int rc = fsys_mkdir(path, 0777);
  if (rc == 0 || !fsys_errno(rc, ENOENT)) {
    return;
  }

  // Slow path
  char *done = path;
  char *p;
  while ((p = strchr(done + 1, '/')) != NULL) {
    *p = '\0';
    rc = fsys_mkdir(path, 0777);
    if (rc != 0 && !fsys_errno(rc, EEXIST)) {
      return;
    }
    *p = '/';
    done = p;
  }
  fsys_mkdir(path, 0777);
}

char *func_mkdirp(const char *nm, unsigned int argc, char **argv) {
  for (unsigned i = 0; i < argc; ++i) {
    char *path = strdup(argv[i]);
    char *saveptr;
    for (char *part = strtok_r(path, " \t", &saveptr);
         part != NULL;
         part = strtok_r(NULL, " \t", &saveptr)) {
      mkdirp(part);
    }
    free(path);
  }
  return NULL;
}

/*
MAKEFILE-TEST-BEGIN

$(shell rm -rf /tmp/egmake-test-mkdirp)
$(EGM.mkdirp /tmp/egmake-test-mkdirp/a/b/c /tmp/egmake-test-mkdirp/a/b/d)

test:
	test -d /tmp/egmake-test-mkdirp/a/b/c
	test -d /tmp/egmake-test-mkdirp/a/b/d
	rm -rf /tmp/egmake-test-mkdirp

MAKEFILE-TEST-END
*/
