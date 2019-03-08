/*
egmake, Enhanced GNU make
Copyright (C) 2014, chys <admin@CHYS.INFO>

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
#include <unistd.h>
#include "fsyscall.h"

char *func_readlink(const char *nm, unsigned int argc, char **argv) {
	if (argc != 1)
		return NULL;
	char *buf = gmk_alloc(PATH_MAX);
	ssize_t l = fsys_readlink(argv[0], buf, PATH_MAX - 1);
	if (l <= 0) {
		gmk_free(buf);
		return NULL;
	} else {
		buf[l] = '\0';
		return buf;
	}
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.readlink /dev/stdin)" = "$(shell readlink /dev/stdin)"

MAKEFILE-TEST-END
*/
