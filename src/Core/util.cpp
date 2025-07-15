#include "util.hpp"
#include "ok.hpp"

namespace xmdb {
void dief(const char *fmt, ...) {
    UZ fmt_len = strlen(fmt);
    char *new_fmt = (char *)calloc(fmt_len + 2, 1);
    memcpy(new_fmt, fmt, fmt_len);
    new_fmt[fmt_len] = '\n';
    fmt = new_fmt;

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
} // namespace xmdb
