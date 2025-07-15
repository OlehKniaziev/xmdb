#ifndef XMDB_UTIL_HPP
#define XMDB_UTIL_HPP

#define TRY(x)                                                                                                          \
    do {                                                                                                                \
        if (!(x)) return {};                                                                                            \
    } while (0)

namespace xmdb {
[[noreturn]]
void dief(const char *fmt, ...);
} // namespace xmdb

#endif // XMDB_UTIL_HPP
