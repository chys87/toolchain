#include <sys/fcntl.h>
#include <sys/uio.h>
#include <tinyx64.h>

#include <initializer_list>

namespace {

void print_cmd(const char *const *args, size_t n) {
  size_t lens[n];
  size_t total_len = 0;
  for (size_t i = 0; i < n; ++i) total_len += lens[i] = strlen(args[i]);

  char buf[total_len + n];

  ChainMemcpy cm(buf);

  for (size_t i = 0; i < n; ++i)
    cm << pair{args[i], lens[i]} << (i == n - 1 ? "\n" : " ");

  fsys_write(2, buf, cm.endptr() - buf);
}

int invoke(const char *exe, std::initializer_list<const char*>base_args,
           size_t argc, char **argv, char **envp) {
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

unsigned get_parallel(unsigned nprocs) { return max(2u, nprocs); }

unsigned get_load_limit(unsigned nprocs) {
  return nprocs + max(2u, nprocs / 3);
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
  ChainMemcpy(j_buf) << "-j" << get_parallel(nprocs) << '\0';

  char l_buf[16];
  ChainMemcpy(l_buf) << "-l" << get_load_limit(nprocs) << '\0';

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
