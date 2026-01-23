#ifndef XMDB_UTIL_HPP
#define XMDB_UTIL_HPP

#include "SourceLocation.hpp"
#include "ok.hpp"

#define TRY(x)                                                                                                          \
    do {                                                                                                                \
        if (!(x)) return {};                                                                                            \
    } while (0)

namespace xmdb {
[[noreturn]]
void dief(const char *fmt, ...);

struct ErrorWithSourceLocation {
    ok::String message;
    SourceLocation location;
};

ok::String to_hex_string(ok::Allocator *, ok::Slice<U8>);
} // namespace xmdb

#endif // XMDB_UTIL_HPP
