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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *func_in_range(const char *nm, unsigned int argc, char **argv) {
  if (argc != 3) return NULL;

  double value = strtod(argv[0], NULL);
  double lo = strtod(argv[1], NULL);
  double hi = strtod(argv[2], NULL);

  char* r = gmk_alloc(2);
  r[0] = (value >= lo && value <= hi) ? '1' : '0';
  r[1] = 0;
  return r;
}

/*
MAKEFILE-TEST-BEGIN

test:
	test "$(EGM.in_range 1, 2, 3)" = "0"
	test "$(EGM.in_range 1, 1, 3)" = "1"
	test "$(EGM.in_range 1, -inf, 3)" = "1"
	test "$(EGM.in_range 1, inf, 3)" = "0"
	test "$(EGM.in_range 1, 1, nan)" = "0"
	test "$(EGM.in_range 1, 1, inf)" = "1"

MAKEFILE-TEST-END
*/
