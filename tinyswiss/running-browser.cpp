#include <fcntl.h>
#include <tinyx64.h>
#include <string_view>
#include <sys/stat.h>

namespace {

constexpr unsigned kMaxAlternatives = 2;

struct Browser {
  const char name[15];
  uint8_t name_len;
  const char key[15];
  uint8_t key_len;
  const char *exe[kMaxAlternatives];
};

const Browser supported_browsers[] = {
    {STR_LEN("Google Chrome"),
     STR_LEN("/chrome"),
     {"google-chrome-stable", "google-chrome"}},
    {STR_LEN("Edge"),
     STR_LEN("/msedge"),
     {"microsoft-edge-stable", "microsoft-edge"}},
    {STR_LEN("Firefox"), STR_LEN("/firefox-bin"), {"firefox", "firefox-bin"}},
};

const Browser *identify_browser(std::string_view exe) {
  for (const Browser &browser: supported_browsers)
    if (browser.key_len <= exe.length() &&
        bcmp(browser.key, exe.data() + exe.length() - browser.key_len,
             browser.key_len) == 0)
      return &browser;
  return nullptr;
}

const Browser *find_running_browser() {
  int fd_proc = fsys_open2("/proc", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd_proc < 0) {
    return nullptr;
  }
  DEFER(fsys_close(fd_proc));

  uid_t my_uid = fsys_getuid();

  const Browser *best_browser = nullptr;

  alignas(linux_dirent64) char buf[16384];
  ssize_t l;
  while ((l = fsys_getdents64(fd_proc, buf, sizeof(buf))) > 0) {
    char *ptr = buf;
    char *ptrend = buf + l;
    do {
      linux_dirent64 *ent = reinterpret_cast<linux_dirent64 *>(ptr);
      DEFER(ptr += ent->d_reclen);

      const char *name = ent->d_name;
      size_t name_l = 0;
      while (name[name_l] != 0 && isdigit(name[name_l]))
        ++name_l;
      if (name[name_l] != 0)
        continue;

      char exe_full_buf[256];
      ssize_t exe_l;

      {
        char proc_exe_path[128];
        ChainMemcpy(proc_exe_path) << pair{name, name_l} << "/exe" << '\0';

        if (struct statx st;
            fsys_statx(fd_proc, proc_exe_path,
                       AT_SYMLINK_NOFOLLOW, STATX_UID, &st) == 0) {
          if (st.stx_uid != my_uid)
            continue;
        } else {
          continue;
        }

        exe_l = fsys_readlinkat(fd_proc, proc_exe_path,
                                exe_full_buf, sizeof(exe_full_buf));
      }
      if (!(exe_l > 0 && exe_l < (ssize_t)sizeof(exe_full_buf)))
        continue;
      if (const Browser *new_match = identify_browser({exe_full_buf,
                                                       size_t(exe_l)})) {
        if (best_browser == nullptr || new_match < best_browser)
          best_browser = new_match;
      }

    } while (ptr < ptrend);
  }

  return best_browser;
}

} // namespace

int running_browser_main(size_t argc, char **argv, char **envp) {

    if (const Browser *browser = find_running_browser()) {

        {
            const char *real_args[argc + 2];
            ChainMemcpy(real_args) << nullptr << pair{argv, argc} << nullptr;

            for (const char *exe: browser->exe) {
                if (!exe)
                    break;
                real_args[0] = exe;
                execvpe(exe, const_cast<char *const *>(real_args), envp);
            }
        }

        char error_msg[128];
        ChainMemcpy cm(error_msg);
        cm << pair{browser->name, size_t(browser->name_len)} <<
          " seems to be running, but we're unable to run it.\n";
        fsys_write(2, error_msg, cm.endptr() - error_msg);
    } else {
        fsys_write(2, STR_LEN("Could not locate any running browser\n"));
    }
    return 127;
}
