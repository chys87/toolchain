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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "fsyscall.h"
#include "util.h"

char* func_cat(const char*, unsigned, char**);

// EGM.shell cache_path,cache_time,cmdline
char* func_shell(const char* nm, unsigned int argc, char** argv) {
  if (argc != 3) {
    return NULL;
  }

  int cache_time = atoi(argv[1]);
  if (cache_time <= 0 || cache_time >= 365 * 86400) return NULL;

  const char* path = argv[0];
  const char* cmdline = argv[2];

  int rc;
  time_t mtime;
  {
    struct statx stx;
    rc = fsys_statx(AT_FDCWD, path, 0, STATX_MTIME, &stx);
    mtime = stx.stx_mtime.tv_sec;
  }

  if (rc != 0 || mtime < time(NULL)) {
    size_t path_len = strlen(path);
    char* tmp_path = malloc(path_len + 16);
    strcpy(mempcpy(tmp_path, path, path_len), ".tmp");

    int fdw = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fdw < 0) return NULL;

    pid_t pid = vfork();
    if (pid == 0) {
      fsys_dup2(fdw, 1);
      const char* args[] = {"sh", "-c", cmdline, NULL};
      execvp("/bin/sh", (char**)args);
      fsys__exit(127);
    }
    if (pid < 0) {
      fsys_close(fdw);
      fsys_unlink(tmp_path);
      return NULL;
    }
    fsys_waitpid(pid, NULL, 0);

    time_t now = time(NULL);
    {
      struct timespec ts[] = {
          {now + cache_time, 0},
          {now + cache_time, 0},
      };
      rc = futimens(fdw, ts);
    }
    if (rc != 0) {
      fsys_close(fdw);
      fsys_unlink(tmp_path);
      return NULL;
    }
    fsys_close(fdw);
    if (fsys_rename(tmp_path, path) != 0) return NULL;
  }

  char* new_argv[] = {(char*)path, NULL};
  return func_cat("EGM.cat", 1, new_argv);
}

/*
MAKEFILE-TEST-BEGIN

$(shell rm -f /tmp/.egmake.shell.cache)

test:
	test "$(EGM.shell /tmp/.egmake.shell.cache,3600,pwd)" = "$(shell pwd)"
	test "$(EGM.shell /tmp/.egmake.shell.cache,3600,ls)" = "$(shell pwd)"

MAKEFILE-TEST-END
*/
