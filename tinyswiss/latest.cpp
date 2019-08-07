#include <fcntl.h>
#include <limits.h>
#include <tinyx32.h>
#include <unistd.h>
#include <string_view>

using namespace std::string_view_literals;

namespace {

struct linux_dirent64 {
  ino64_t        d_ino;
  off64_t        d_off;
  unsigned short d_reclen;
  unsigned char  d_type; // In linux_dirent, it's after d_name;
                         // but in linux_dirent64, it's here.
  char           d_name[1];

  unsigned char get_d_type() const { return d_type; }
};

using DirEnt = linux_dirent64;


int strnumcmp(const char *a, const char *b) noexcept {
  const uint8_t *u = (const uint8_t *)a;
  const uint8_t *v = (const uint8_t *)b;
  unsigned U, V;

  do {
    U = *u++;
    V = *v++;
  } while ((U == V) && V);
  if (U == V)
    return 0;

  int cmpresult = U - V;

  // Both are digits - Compare as numbers:
  // We use the following rule:
  // If the two series of digits are of unequal length,
  // the longer one is larger;
  // otherwise, the comparison is straightforward
  for (;;) {
    U -= '0';
    V -= '0';
    if (U < 10) {
      if (V >= 10) {
        // Only U is a digit
        return 1;
      }
    } else if (V < 10) {
      // Only V is a digit
      return -1;
    } else {
      // Neither is a digit
      return cmpresult;
    }
    U = *u++;
    V = *v++;
  }
}

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

    char buf[65536];
    ssize_t l;
    while ((l = fsys_getdents64(fddir, buf, sizeof(buf))) > 0) {
      char *ptr = buf;
      char *ptrend = buf + l;
      do {
        DirEnt *ent = reinterpret_cast<DirEnt *>(ptr);

        if (strncmp(ent->d_name, prefix.data(), prefix.length()) == 0) {
          size_t d_name_len = prefix.length() +
            strlen(ent->d_name + prefix.length());
          if (!all_version_digits({ent->d_name + prefix.length(),
                                   d_name_len - prefix.length()}))
            continue;
          if (fsys_faccessat(fddir, ent->d_name, X_OK, 0) == 0) {
            if (d_name_len + path.length() < PATH_MAX &&
                strnumcmp(last_base_name, ent->d_name) < 0) {
              ChainMemcpy(last_name) << path <<
                std::string_view{ent->d_name, d_name_len} << '\0';
              last_base_name = last_name + path.length();
            }
          }
        }

      } while ((ptr += reinterpret_cast<DirEnt *>(ptr)->d_reclen) < ptrend);
    }

    fsys_close(fddir);
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
