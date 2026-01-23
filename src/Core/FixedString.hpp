#pragma once

#include "ok.hpp"

namespace xmdb {
struct FixedString {
    static constexpr UZ DATA_SIZE = 63;
    static constexpr UZ PREFIX_SIZE = 1;

    U8 count;
    U8 items[DATA_SIZE];
};

static_assert(sizeof(FixedString::count) == FixedString::PREFIX_SIZE);
static_assert(sizeof(FixedString) == FixedString::PREFIX_SIZE + FixedString::DATA_SIZE);

FixedString create_fixed_string(ok::StringView);

static inline ok::StringView view(ok::Allocator *a, FixedString fs) {
    U8 *buffer = a->alloc<U8>(fs.count);
    memcpy(buffer, fs.items, fs.count);
    return {(const char *)buffer, fs.count};
}
}
