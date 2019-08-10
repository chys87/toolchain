#include <string_view>
#include <sys/types.h>

int strnumcmp(const char *, const char *) __attribute__((__pure__));
ssize_t write_sv(int fd, std::string_view sv);
