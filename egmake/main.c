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

int plugin_is_GPL_compatible __attribute__((visibility("default")));

typedef char *gmk_func_type(const char *, unsigned, char **);

gmk_func_type func_cat;
gmk_func_type func_clang_march_native;
gmk_func_type func_cpus;
gmk_func_type func_expanduser;
gmk_func_type func_hostname;
gmk_func_type func_in_range;
gmk_func_type func_mkdirp;
gmk_func_type func_pwd;
gmk_func_type func_readlink;
gmk_func_type func_relpath;

int egmake_gmk_setup(void) __attribute__((visibility("default")));
int egmake_gmk_setup(void) {
  gmk_add_function("EGM.cat", &func_cat, 1, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.clang_march_native", &func_clang_march_native, 0, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.cpus", &func_cpus, 0, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.expanduser", &func_expanduser, 1, 1, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.hostname", &func_hostname, 0, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.in_range", &func_in_range, 3, 3, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.mkdirp", &func_mkdirp, 0, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.pwd", &func_pwd, 0, 0, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.readlink", &func_readlink, 1, 1, GMK_FUNC_DEFAULT);
  gmk_add_function("EGM.relpath", &func_relpath, 2, 2, GMK_FUNC_DEFAULT);
  return 1;
}
