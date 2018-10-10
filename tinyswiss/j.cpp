#include <fcntl.h>
#include <initializer_list>
#include <tinyx32.h>
#include <unistd.h>
#include <sys/uio.h>

namespace {

int invoke(const char *exe, std::initializer_list<const char*>base_args, size_t argc, char **argv, char **envp) {
    char *real_args[base_args.size() + argc + 1];
    char **pra = real_args;
    pra = (char **)mempcpy(pra, base_args.begin(), base_args.size() * sizeof(char *));
    pra = (char **)mempcpy(pra, argv, argc * sizeof(char *));
    *pra = nullptr;

    execvpe(exe, real_args, envp);

    {
        struct iovec vec[3] = {
            {(char *)STR_LEN("Failed to execute command ")},
            {const_cast<char *>(exe), strlen(exe)},
            {(char *)STR_LEN("\n")},
        };
        fsys_writev(2, vec, 3);
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
