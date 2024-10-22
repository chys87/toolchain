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

int latest(std::string_view prefix, std::string_view suffix, size_t argc,
           char **argv, char **envp) {
  static constexpr size_t kPathMax = 128;
  char last_name[kPathMax];
  char* last_basename_ptr = nullptr;

  uint64_t last_path_key = 0;

  static const char paths[] = "\x0e/usr/local/bin\0\x08/usr/bin\0\x04/bin\0";

  for (const char* pp = paths; *pp; ) {
    const char* path = pp + 1;
    uint8_t path_len = *pp;
    pp += size_t(path_len) + 2;

    int fddir = fsys_open2(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fddir < 0) continue;

    DEFER(fsys_close(fddir));

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

    fsys_aux::readdir<fsys_aux::ReadDirNoSkipDots,
                      fsys_aux::ReadDirBufSize<65536>>(fddir, [&](DirEnt *ent) {
      if (ent->d_type != DT_UNKNOWN && ent->d_type != DT_LNK &&
          ent->d_type != DT_REG)
        return;
      if (bcmp(ent->d_name, prefix.data(), prefix.length()) == 0) {
        size_t d_name_len =
            prefix.length() + strlen(ent->d_name + prefix.length());
        if (d_name_len + path_len >= kPathMax - 1) return;
        if (d_name_len < prefix.size() + suffix.size()) return;
        if (!suffix.empty() && bcmp(ent->d_name + d_name_len - suffix.size(),
                                    suffix.data(), suffix.size()) != 0)
          return;
        if (!all_version_digits(
                {ent->d_name + prefix.length(),
                 d_name_len - prefix.length() - suffix.length()}))
          return;
        if ((last_basename_ptr == nullptr ||
             strnumcmp(last_basename_ptr, ent->d_name) < 0) &&
            fsys_faccessat(fddir, ent->d_name, X_OK, 0) == 0) {
          last_basename_ptr = (char*)mempcpy(last_name, path, path_len);
          *last_basename_ptr++ = '/';
          memcpy(last_basename_ptr, ent->d_name, d_name_len + 1);
        }
      }
    });
  }

  if (last_basename_ptr) {
    // We can't use execveat - that doesn't work if the target has a "#!" interpreter
    argv[-1] = last_name;
    fsys_execve(last_name, argv - 1, envp);
    return 127;
  }
  return 1;
}

} // namespace

int python3_latest_main(size_t argc, char **argv, char **envp) {
  return latest("python3."sv, {}, argc, argv, envp);
}

int python3_latest_config_main(size_t argc, char **argv, char **envp) {
  return latest("python3."sv, "-config"sv, argc, argv, envp);
}

int gcc_latest_main(size_t argc, char **argv, char **envp) {
  return latest("gcc"sv, {}, argc, argv, envp);
}

int gxx_latest_main(size_t argc, char **argv, char **envp) {
  return latest("g++"sv, {}, argc, argv, envp);
}
