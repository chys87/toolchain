#include <tinyx64.h>
#include "utils.h"

int j_main(size_t argc, char **argv, char **);
int mvln_main(size_t argc, char **argv, char **);
int python3_latest_main(size_t, char **, char **);
int python_latest_main(size_t, char **, char **);
int running_browser_main(size_t, char **, char **);

namespace {

constexpr size_t kMaxSubCommandNameLen = 22;

struct Dispatch {
  char name[kMaxSubCommandNameLen + 1];
  uint8_t name_len;
  int (*func)(size_t argc, char **, char **);
};

constexpr Dispatch dispatch[] = {
  {STR_LEN("mvln"), mvln_main},
  {STR_LEN("j"), j_main},
  {STR_LEN("python3-latest"), python3_latest_main},
  {STR_LEN("python-latest"), python_latest_main},
  {STR_LEN("running-browser"), running_browser_main},
};

void unknown_subcommand() {
  fsys_write(2, STR_LEN("Unknown subcommand\n"));
}

void print_subcommands() {
  fsys_write(1, STR_LEN("Available subcommands:\n"));
  for (const Dispatch &disp: dispatch) {
    char buf[kMaxSubCommandNameLen + 1];
    memcpy(mempcpy(buf, disp.name, disp.name_len), "\n", 1);
    fsys_write(1, buf, disp.name_len + 1);
  }
}

} // namespace

extern "C" int main(int argc, char **argv, char **envp) {
  const char *name = argv[0];
  if (name == NULL) {
    unknown_subcommand();
    return 1;
  }
  const char *bname = basename(name);
  if (strncmp(bname, "TS", 2) == 0 && (bname[2] == '\0' || bname[2] == '.')) {
    --argc;
    ++argv;
    name = *argv;
    if (name == NULL || strcmp(name, "--help") == 0) {
      print_subcommands();
      return 0;
    }
    bname = basename(name);
  }

  size_t bname_len = strlen(bname);
  for (const Dispatch &disp: dispatch) {
    if (disp.name_len == bname_len && memcmp(disp.name, bname, bname_len) == 0)
      return disp.func(argc - 1, argv + 1, envp);
  }

  unknown_subcommand();
  return 1;
}
