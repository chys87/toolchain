#include <tinyx32.h>

int mvln_main(char **argv);

namespace {

struct Dispatch {
    const char *name;
    size_t name_len;
    int (*func)(char **);
};

static const Dispatch dispatch[] = {
    {STR_LEN("mvln"), mvln_main},
};

void unknown_subcommand() __attribute__((noreturn));
void unknown_subcommand() {
    fsys_write(2, STR_LEN("Unknown subcommand\n"));
    exit(1);
}

void print_subcommands() {
    fsys_write(1, STR_LEN("Available subcommands:\n"));
    for (const Dispatch &disp: dispatch) {
        char buf[disp.name_len + 1];
        memcpy(mempcpy(buf, disp.name, disp.name_len), "\n", 1);
        fsys_write(1, buf, disp.name_len + 1);
    }
}

const char *basename(const char *s) {
    const char *r = s;
    while (*s) {
        if (*s == '/')
            r = s + 1;
        ++s;
    }
    return r;
}

} // namespace

int main(int argc, char **argv) {
    const char *name = argv[0];
    if (name == NULL)
        unknown_subcommand();
    const char *bname = basename(name);
    if (strcmp(bname, "TS") == 0) {
        ++argv;
        name = *argv;
        if (name == NULL) {
            print_subcommands();
            return 0;
        }
        bname = basename(name);
    }

    for (const Dispatch &disp: dispatch) {
        if (strcmp(disp.name, bname) == 0) {
            return disp.func(argv + 1);
        }
    }

    return 0;
}
