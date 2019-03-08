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

char *func_pwd(const char *nm, unsigned int argc, char **argv) {
	char *buf = gmk_alloc(PATH_MAX);
	if (getcwd(buf, PATH_MAX) == NULL)
		buf[0] = '\0';
	return buf;
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.pwd )" = "$(shell pwd)"

MAKEFILE-TEST-END
*/
