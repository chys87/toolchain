#include <errno.h>
#include <fcntl.h>
#include <initializer_list>
#include <tinyx32.h>
#include <unistd.h>
#include <string_view>
#include <sys/uio.h>
#include "utils.h"

using namespace std::string_view_literals;

namespace {

using DirEnt = fsys_linux_dirent64;

void print_cmd(const char *const *args, size_t n) {
  size_t lens[n];
  size_t total_len = 0;
  for (size_t i = 0; i < n; ++i)
    total_len += lens[i] = strlen(args[i]);

  char buf[total_len + n];

  ChainMemcpy cm(buf);

  for (size_t i = 0; i < n; ++i)
    cm << pair{args[i], lens[i]} << (i == n - 1 ? "\n" : " ");

  fsys_write(2, buf, cm.endptr() - buf);
}

struct Compiler {
  char *path = nullptr;
  uint32_t priority = 0;

  ~Compiler() { delete[] path; }
  void replace(std::string_view cur_dir, const char *cur_name,
               uint32_t cur_priority);
};

void Compiler::replace(std::string_view cur_dir, const char *cur_name,
                       uint32_t cur_priority) {
  if (cur_priority < priority) {
    return;
  }

  if (path && cur_priority == priority &&
      strnumcmp(basename(path), cur_name) >= 0) {
    return;
  }

  size_t cur_name_l = strlen(cur_name);
  char *new_path = new char[cur_dir.size() + cur_name_l + 2];
  ChainMemcpy(new_path) << cur_dir << '/' << pair{cur_name, cur_name_l} << '\0';
  delete[] std::exchange(path, new_path);
  priority = cur_priority;
}

struct CompilerPreference {
  const char *name;
  uint8_t prefix_len;
  uint8_t priority;
};

const CompilerPreference cc_preference[] = {
  {"gcc-svn-opt", 0, 10},
  {"gcc-svn", 0, 9},
  {"gcc-", 4, 8},
  {"gcc", 0, 7},
  {"cc", 0, 6},
};

const CompilerPreference cxx_preference[] = {
  {"g++-svn-opt", 0, 10},
  {"g++-svn", 0, 9},
  {"g++-", 4, 8},
  {"g++", 0, 7},
  {"c++", 0, 6},
};

void guess_latest_compiler(char **envp) {
  char **ppath = nullptr;
  for (char **p = envp; *p; ++p) {
    char *env = *p;
    if (env && strncmp(env, "PATH=", 5) == 0) {
      ppath = p;
      break;
    }
  }

  if (ppath == nullptr) {
    write_sv(2, "$PATH not found. Likely a serious problem.\n"sv);
    return;
  }

  std::string_view link_dir_prefix = "/tmp/tinyswiss-j-compilers-";
  char link_dir_z[link_dir_prefix.length() + 32];
  ChainMemcpy cm(link_dir_z);
  cm << link_dir_prefix << fsys_getuid() << '\0';
  std::string_view link_dir_sv(link_dir_z, cm.endptr() - 1 - link_dir_z);

  Compiler gcc;
  Compiler gxx;

  const char *PATH = *ppath + 5;
  const char *e = PATH;
  for (const char *p = PATH; *p; p = *e ? e + 1 : e) {
    e = strchrnul(p, ':');
    if (p == e || *p != '/') {
      // Non-absolute paths are supported by the system, but are highly
      // dangerous and we don't attempt to support them
      continue;
    }
    std::string_view cur_dir(p, e - p);
    if (cur_dir == link_dir_sv) {
      // link_dir already in PATH. Skip
      write_sv(2, "/tmp/tinyswiss-j-compilers already in PATH.");
      return;
    }

    char cur_dir_z[cur_dir.size() + 1];
    ChainMemcpy(cur_dir_z) << cur_dir << '\0';

    int fddir = fsys_open2(cur_dir_z, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fddir < 0)
      continue;

    fsys_aux::readdir<fsys_aux::ReadDirClose, fsys_aux::ReadDirNoSkipDots,
                      fsys_aux::ReadDirBufSize<65536>>(fddir, [&](DirEnt *ent) {
      const char *d_name = ent->d_name;
      for (const CompilerPreference &cp: cc_preference) {
        bool match = false;
        if (cp.prefix_len) {
          match = (strncmp(d_name, cp.name, cp.prefix_len) == 0 &&
                   isdigit(d_name[cp.prefix_len]));
        } else {
          match = (strcmp(d_name, cp.name) == 0);
        }
        if (match && fsys_faccessat(fddir, d_name, X_OK, 0) == 0) {
          gcc.replace(cur_dir, d_name, cp.priority);
          break;
        }
      }
      for (const CompilerPreference &cp: cxx_preference) {
        bool match = false;
        if (cp.prefix_len) {
          match = (strncmp(d_name, cp.name, cp.prefix_len) == 0 &&
                   isdigit(d_name[cp.prefix_len]));
        } else {
          match = (strcmp(d_name, cp.name) == 0);
        }
        if (match && fsys_faccessat(fddir, d_name, X_OK, 0) == 0) {
          gxx.replace(cur_dir, d_name, cp.priority);
          break;
        }
      }
    });
  }

  if (gcc.path || gxx.path) {
    int rc = fsys_mkdir(link_dir_z, 0700);
    if (rc != 0 && !fsys_errno(rc, EEXIST)) {
      write_sv(2, "Failed to create a directory for selected compilers.\n");
      return;
    }

    int fddir = fsys_open2(link_dir_z, O_PATH | O_DIRECTORY | O_CLOEXEC);
    if (fddir < 0) {
      write_sv(2, "Failed to open directory for selected compilers.\n");
      return;
    }

    if (gcc.path) {
      fsys_unlinkat(fddir, "gcc.tmp", 0);
      fsys_symlinkat(gcc.path, fddir, "gcc.tmp");
      fsys_renameat(fddir, "gcc.tmp", fddir, "gcc");
    }
    if (gxx.path) {
      fsys_unlinkat(fddir, "g++.tmp", 0);
      fsys_symlinkat(gxx.path, fddir, "g++.tmp");
      fsys_renameat(fddir, "g++.tmp", fddir, "g++");
    }
    fsys_close(fddir);

    size_t cur_PATH_len = strlen(PATH);
    char *new_PATH = new char[5 + cur_PATH_len + link_dir_sv.length() + 2];
    ChainMemcpy(new_PATH) << "PATH=" << link_dir_sv << ':' <<
      pair{PATH, cur_PATH_len} << '\0';
    write_sv(2, new_PATH);
    write_sv(2, "\n");
    *ppath = new_PATH;
  }

}

int invoke(const char *exe, std::initializer_list<const char*>base_args,
           size_t argc, char **argv, char **envp) {

  guess_latest_compiler(envp);

  const char *real_args[base_args.size() + argc + 1];

  ChainMemcpy(real_args) << pair{base_args.begin(), base_args.size()} <<
    pair{argv, argc} << nullptr;

  print_cmd(real_args, base_args.size() + argc);

  execvpe(exe, const_cast<char *const *>(real_args), envp);

  {
    const char *prefix = "Failed to execute command ";
    const char *suffix = "\n";
    size_t exe_len = strlen(exe);
    char buf[strlen(prefix) + exe_len + strlen(suffix)];

    ChainMemcpy(buf) << prefix << pair{exe, exe_len} << suffix;

    fsys_write(2, buf, strlen(prefix) + exe_len + strlen(suffix));
  }

  return 127;
}

} // namespace

int j_main(size_t argc, char **argv, char **envp) {

  int fd_null = fsys_open2("/dev/null", O_RDONLY | O_CLOEXEC);
  if (fd_null >= 0)
    fsys_dup2(fd_null, 0);

  if (fsys_access("build.py", X_OK) == 0)
    return invoke("./build.py", {"./build.py", "-v"}, argc, argv, envp);

  if (fsys_access("build.ninja", R_OK) == 0)
    return invoke("ninja", {"ninja", "-v"}, argc, argv, envp);

  if (fsys_access("BUILD", R_OK) == 0)
    return invoke("bazel", {"bazel", "build"}, argc, argv, envp);

  // Make - we need to determine the number of processors ourselves
  unsigned nprocs = get_nprocs();
  if (nprocs == 0)
    nprocs = 1;

  char j_buf[16];
  ChainMemcpy(j_buf) << "-j" << nprocs << '\0';

  char l_buf[16];
  ChainMemcpy(l_buf) << "-l" << nprocs + 1 << '\0';

  // Make with "-f"
  for (const char *filename: {"Makefile.test"}) {
    if (fsys_access(filename, R_OK) == 0) {
      return invoke("make", {"make", "-f", filename, j_buf, l_buf},
                    argc, argv, envp);
    }
  }

  // Make: the documented order is GNUmakefile, makefile, Makefile, but we
  // test Makefile first since it's the most common
  for (const char *filename: {"Makefile", "GNUmakefile", "makefile"}) {
    if (fsys_access(filename, R_OK) == 0) {
      return invoke("make", {"make", j_buf, l_buf}, argc, argv, envp);
    }
  }

  fsys_write(2, STR_LEN("No supported build file found\n"));
  return 127;
}
