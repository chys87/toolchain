#include "tinyx32.h"
#include <sys/errno.h>

int execvpe(const char *exe, char *const *args, char *const *envp) {
    if (strchr(exe, '/'))
        return fsys_execve(exe, args, envp);

    const char *path = "/usr/local/bin/:/usr/bin/:/bin";
    for (char *const *e = envp; *e; ++e) {
        if (strncmp(*e, "PATH=", 5) == 0) {
            path = *e + 5;
            break;
        }
    }

    int ret = -ENOENT;
    size_t exe_base_len = strlen(exe);
    for (const char *p = path; *p; ) {
        const char *sep = strchrnul(p, ':');
        size_t l = sep - p;
        char real_exe[l + 1 + exe_base_len + 2];
        char *w = real_exe;
        if (l)
            w = (char *)mempcpy(w, p, l);
        else
            *w++ = '.';
        *w++ = '/';
        w = (char *)mempcpy(w, exe, exe_base_len);
        *w++ = '\0';
        ret = fsys_execve(real_exe, args, envp);
    }
    return ret;
}
