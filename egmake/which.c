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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fsyscall.h"
#include "util.h"

char* func_which(const char* nm, unsigned int argc, char** argv) {
  if (argc != 1) return NULL;
  const char* exe = argv[0];
  size_t l = strlen(exe);
  if (l == 0) return NULL;
  do {
    if (memchr(exe, '/', l)) break;

    const char* path = getenv("PATH");
    if (path == NULL) break;

    const char* s = path;
    while (true) {
      const char* sp = strchrnul(s, ':');
      size_t spl = sp - s;
      if (spl == 0) {
        if (fsys_access(exe, X_OK) == 0) break;
      } else {
        char new_path[spl + 1 + l + 1];
        char* w = new_path;
        w = Mempcpy(w, s, spl);
        *w++ = '/';
        w = Mempcpy(w, exe, l);
        *w = '\0';
        if (fsys_access(new_path, X_OK) == 0) {
          return strdup_to_gmk_with_len(new_path, spl + 1 + l);
        }
      }
      s = sp;
      if (*s == '\0') break;
      ++s;
    }
  } while (false);
  return strdup_to_gmk_with_len(exe, l);
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.which ls)" = "$(shell which ls)"

MAKEFILE-TEST-END
*/
