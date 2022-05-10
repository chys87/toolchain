#include <limits.h>
#include <sys/fcntl.h>
#include <tinyx32.h>

#include <string_view>

#include "utils.h"

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

using namespace std::string_view_literals;

namespace {

using DirEnt = fsys_linux_dirent64;

bool all_version_digits(std::string_view s) {
  for (char c: s)
    if (!isdigit(c) && c != '.' && c != '-')
      return false;
  return true;
}

int latest(std::string_view prefix, size_t argc, char **argv, char **envp) {

  char last_name[PATH_MAX];
  char *last_base_name = last_name;
  last_name[0] = 0;

  for (std::string_view path: {"/usr/local/bin/"sv, "/usr/bin/"sv, "/bin/"sv}) {
    int fddir = fsys_open2(path.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fddir < 0)
      continue;

    fsys_aux::readdir<fsys_aux::ReadDirClose, fsys_aux::ReadDirNoSkipDots,
                      fsys_aux::ReadDirBufSize<65536>>(fddir, [&](DirEnt *ent) {
      if (strncmp(ent->d_name, prefix.data(), prefix.length()) == 0) {
        size_t d_name_len = prefix.length() +
          strlen(ent->d_name + prefix.length());
        if (!all_version_digits({ent->d_name + prefix.length(),
                                 d_name_len - prefix.length()}))
          return;
        if (fsys_faccessat(fddir, ent->d_name, X_OK, 0) == 0) {
          if (d_name_len + path.length() < PATH_MAX &&
              strnumcmp(last_base_name, ent->d_name) < 0) {
            ChainMemcpy(last_name) << path <<
              std::string_view{ent->d_name, d_name_len} << '\0';
            last_base_name = last_name + path.length();
          }
        }
      }
    });
  }

  if (last_name[0] != '\0') {
    char *newargv[argc + 2];
    ChainMemcpy(newargv) << last_base_name << pair{argv, argc} << nullptr;
    fsys_execve(last_name, newargv, envp);
  }
  return 127;
}

} // namespace

int python3_latest_main(size_t argc, char **argv, char **envp) {
  return latest("python3.", argc, argv, envp);
}

int python_latest_main(size_t argc, char **argv, char **envp) {
  return latest("python", argc, argv, envp);
}
