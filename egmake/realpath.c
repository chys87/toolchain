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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *func_realpath(const char *nm, unsigned int argc, char **argv) {
	if (argc != 1)
		return NULL;
	char *path = realpath(argv[0], NULL);
	if (path == NULL)
		return NULL;
	size_t l = strlen(path) + 1;
	char *res = memcpy(gmk_alloc(l), path, l);
	free(path);
	return res;
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.realpath /usr/lib/../bin/make)" = "$(shell readlink -f /usr/lib/../bin/make)"

MAKEFILE-TEST-END
*/
