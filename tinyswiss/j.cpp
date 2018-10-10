#include <fcntl.h>
#include <initializer_list>
#include <tinyx32.h>
#include <unistd.h>
#include <sys/uio.h>

namespace {

int invoke(const char *exe, std::initializer_list<const char*>base_args, size_t argc, char **argv, char **envp) {
    const char *real_args[base_args.size() + argc + 1];

    ChainMemcpy(real_args) << pair{base_args.begin(), base_args.size()} << pair{argv, argc} << nullptr;

    execvpe(exe, const_cast<char *const *>(real_args), envp);

    {
        const char *prefix = "Failed to execute command ";
        const char *suffix = "\n";
        size_t exe_len = strlen(exe);
        char buf[strlen(prefix) + exe_len + strlen(suffix)];

        ChainMemcpy(buf) << pair{prefix, strlen(prefix)} << pair{exe, exe_len} << pair{suffix, strlen(suffix)};

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
        return invoke("./build.py", {"build.py", "-v"}, argc, argv, envp);

    if (fsys_access("build.ninja", R_OK) == 0)
        return invoke("ninja", {"ninja", "-v"}, argc, argv, envp);

    // TODO: Compute the number of available processors
    return invoke("make", {"make", "-j4", "-l5"}, argc, argv, envp);
}
