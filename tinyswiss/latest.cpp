#include <dirent.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <tinyx64.h>

#include <string_view>

#include "utils.h"

using namespace std::string_view_literals;

namespace {

using DirEnt = fsys_linux_dirent64;

bool all_version_digits(std::string_view s) {
  for (char c : s)
    if (!isdigit(c) && c != '.' && c != '-') return false;
  return true;
}

int latest(std::string_view prefix, size_t argc, char **argv, char **envp) {
  static constexpr size_t kPathMax = 128;
  int last_fddir = -1;
  char last_name[kPathMax];

  uint64_t last_path_key = 0;

  for (const char* path : {"/usr/local/bin", "/usr/bin", "/bin"}) {
    int fddir = fsys_open2(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fddir < 0) continue;

    {
      struct statx stx;
      int rc = fsys_statx(fddir, "", AT_EMPTY_PATH | AT_STATX_DONT_SYNC, STATX_INO, &stx);
      if (rc == 0) {
        uint64_t path_key =
            (stx.stx_dev_major | ((uint64_t)stx.stx_dev_minor << 32)) ^
            stx.stx_ino;
        if (path_key == last_path_key) continue;
        last_path_key = path_key;
      }
    }

    // Don't bother to close fddir
    fsys_aux::readdir<fsys_aux::ReadDirNoSkipDots,
                      fsys_aux::ReadDirBufSize<65536>>(fddir, [&](DirEnt *ent) {
      if (ent->d_type != DT_UNKNOWN && ent->d_type != DT_LNK &&
          ent->d_type != DT_REG)
        return;
      if (bcmp(ent->d_name, prefix.data(), prefix.length()) == 0) {
        size_t d_name_len =
            prefix.length() + strlen(ent->d_name + prefix.length());
        if (d_name_len >= kPathMax) return;
        if (!all_version_digits(
                {ent->d_name + prefix.length(), d_name_len - prefix.length()}))
          return;
        if (strnumcmp(last_name, ent->d_name) < 0 &&
            fsys_faccessat(fddir, ent->d_name, X_OK, 0) == 0) {
          last_fddir = fddir;
          memcpy(last_name, ent->d_name, d_name_len + 1);
        }
      }
    });
  }

  if (last_fddir >= 0) {
    argv[-1] = last_name;
    fsys_execveat(last_fddir, last_name, argv - 1, envp, 0);
  }
  return 127;
}

} // namespace

int python3_latest_main(size_t argc, char **argv, char **envp) {
  return latest("python3."sv, argc, argv, envp);
}

int python_latest_main(size_t argc, char **argv, char **envp) {
  return latest("python"sv, argc, argv, envp);
}

int gcc_latest_main(size_t argc, char **argv, char **envp) {
  return latest("gcc"sv, argc, argv, envp);
}

int gxx_latest_main(size_t argc, char **argv, char **envp) {
  return latest("g++"sv, argc, argv, envp);
}
