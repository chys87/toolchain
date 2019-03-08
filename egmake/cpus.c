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
#include <stdio.h>
#include <sys/sysinfo.h>

// Count the number of logical processors
char *func_cpus(const char *nm, unsigned int argc, char **argv) {
	unsigned cpu_cnt = get_nprocs();
	char *res = gmk_alloc(8);
	snprintf(res, 8, "%u", cpu_cnt);
	return res;
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.cpus )" -eq "`egrep '^processor\s*:' /proc/cpuinfo | wc -l`"

MAKEFILE-TEST-END
*/
