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

#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <gnumake.h>

#include "util.h"

#define PWBUFSIZ 8192

static char *expand_current_user(const char *trail, size_t trail_l) {
  const char *home = getenv("HOME");

  char buffer[PWBUFSIZ];
  if (home == NULL) {
    struct passwd *result;
    struct passwd pwd;
    if (getpwuid_r(getuid(), &pwd, buffer, sizeof(buffer), &result) != 0)
      return NULL;
    if (result == NULL)  // No error, but user is not found.
      return NULL;
    home = result->pw_dir;
    if (home == NULL)
      return NULL;
  }

  size_t home_l = strlen(home);
  char *p = gmk_alloc(home_l + trail_l + 1);
  memcpy(Mempcpy(p, home, home_l), trail, trail_l + 1);
  return p;
}

static char *expand_explicit_user(const char *user, size_t user_l,
                                  const char *trail, size_t trail_l) {
  char *user_z = malloc(user_l + 1);
  *Mempcpy(user_z, user, user_l) = '\0';

  char buffer[PWBUFSIZ];
  struct passwd pwd;
  struct passwd *result;
  int ret = getpwnam_r(user_z, &pwd, buffer, sizeof(buffer), &result);
  free(user_z);

  if (ret != 0)
    return NULL;
  if (result == NULL)  // No error, but user is not found.
    return NULL;

  const char *home = result->pw_dir;
  size_t home_l = strlen(home);
  char *p = gmk_alloc(home_l + trail_l + 1);
  memcpy(Mempcpy(p, home, home_l), trail, trail_l + 1);
  return p;
}

char *func_expanduser(const char *nm, unsigned int argc, char **argv) {
  if (argc != 1)
    return NULL;

  const char *path = argv[0];
  if (path == NULL || *path == '\0')
    return NULL;

  size_t l = strlen(path);
  if (*path == '~') {
    if (path[1] == '/' || path[1] == '\0') {
      char *r = expand_current_user(path + 1, l - 1);
      if (r != NULL)
        return r;
    } else {
      size_t username_len = 1;
      while (path[username_len + 1] != '/' && path[username_len + 1] != '\0')
        ++username_len;

      char *r = expand_explicit_user(
          path + 1, username_len,
          path + 1 + username_len, l - 1 - username_len);
      if (r != NULL)
        return r;
    }
  }
  return strdup_to_gmk_with_len(path, l);
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.expanduser ~)" = "$$HOME"
	test "$(EGM.expanduser ~/)" = "$$HOME/"
	test "$(EGM.expanduser ~/bin)" = "$$HOME/bin"
	test "$(EGM.expanduser ~idontknowthisguy)" = "~idontknowthisguy"
	test "$(EGM.expanduser ~root)" = "/root"
	test "$(EGM.expanduser ~root/)" = "/root/"
	test "$(EGM.expanduser ~root/bin)" = "/root/bin"

MAKEFILE-TEST-END
*/
