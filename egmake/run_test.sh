#!/bin/bash
# egmake, Enhanced GNU make
# Copyright (C) 2014, chys <admin@CHYS.INFO>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -e

if [[ ! -r "$1" ]]; then
	echo "You must specify a filename." >&2
	exit 1
fi

tmpfile="`mktemp`"
trap 'rm -f "$tmpfile"' EXIT

{
	echo "load $PWD/egmake.so"
	echo ".PHONY: test"
	echo "test:"
	echo ".SUFFIXES :="

	sed -n -e '
	/MAKEFILE-TEST-BEGIN/,/MAKEFILE-TEST-END/ { /MAKEFILE-TEST-\(BEGIN\|END\)/d; p }
	' $1
} >"$tmpfile"

${MAKE:-make} --no-print-directory -f "$tmpfile"
