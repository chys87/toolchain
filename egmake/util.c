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
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <gnumake.h>

void strtr_in_place(char *s, size_t l, char from, char to) {
  char *e = s + l;
  while (s < e) {
    char *p = memchr(s, from, e - s);
    if (p == NULL)
      break;
    *p = to;
    s = p + 1;
  }
}

void replace_cr_ln_in_place(char *s, size_t l) {
  strtr_in_place(s, l, '\r', ' ');
  strtr_in_place(s, l, '\n', ' ');
}

char *Memcpy(char *d, const void *s, size_t n) {
  return (char *)memcpy(d, s, n);
}

char *Mempcpy(char *d, const void *s, size_t n) {
  return (char *)mempcpy(d, s, n);
}

char *strdup_to_gmk_with_len(const char *s, size_t n) {
  char *ret = (char *)gmk_alloc(n + 1);
  *Mempcpy(ret, s, n) = '\0';
  return ret;
}

char *strdup_to_gmk(const char *s) {
  return (char *)strdup_to_gmk_with_len(s, strlen(s));
}
