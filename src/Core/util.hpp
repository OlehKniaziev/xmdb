#pragma once

#include "SourceLocation.hpp"
#include "ok.hpp"

#if !defined(OK_NO_STDLIB)
#include <common.h>
#endif // !OK_NO_STDLIB

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

#if !defined(OK_NO_STDLIB)
struct WebArenaAllocator : public ok::Allocator {
    explicit WebArenaAllocator(web_arena *impl) : impl{impl} {}

    void *raw_alloc(UZ size) override {
        return WebArenaPush(impl, size);
    }

    void raw_dealloc(void *ptr, UZ size) override {
        (void) ptr;
        (void) size;
    }

    void *raw_resize(void *ptr, UZ old_size, UZ new_size) override {
        return WebArenaRealloc(impl, ptr, old_size, new_size);
    }

    web_arena *impl;
};
#endif // !OK_NO_STDLIB
} // namespace xmdb
