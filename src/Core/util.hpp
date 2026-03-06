#pragma once

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
ok::Optional<ok::Slice<U8>> from_hex_string(ok::Allocator *, ok::StringView);
} // namespace xmdb
