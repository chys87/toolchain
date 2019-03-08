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
#pragma once

#include <stddef.h>
#include <string.h>

void strtr_in_place(char *s, size_t l, char from, char to);

// Replace '\r' and '\n' with spaces
void replace_cr_ln_in_place(char *, size_t);

static inline size_t max_size(size_t a, size_t b) {
	return (a < b) ? b : a;
}

char *Memcpy(char *d, const void *s, size_t n);
char *Mempcpy(char *d, const void *s, size_t n);
