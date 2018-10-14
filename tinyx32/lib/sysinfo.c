#include "tinyx32.h"
#include <fcntl.h>

unsigned get_nprocs() {
    char buf[128];
    int fd = fsys_open2("/sys/devices/system/cpu/online", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return 0;

    ssize_t l = fsys_read(fd, buf, sizeof(buf) - 1);
    fsys_close(fd);
    if (l <= 0)
        return 0;

    buf[l] = '\0';

    char *p = buf;
    unsigned cnt = 0;
    for (;;) {
        while (*p && !isdigit(*p))
            ++p;
        if (!*p)
            break;
        unsigned m = strtou(p, &p);
        if (*p == '-') {
            ++p;
            unsigned n = strtou(p, &p);
            cnt += n - m + 1;
        } else {
            ++cnt;
        }
    }
    return cnt;
}
