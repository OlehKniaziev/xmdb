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

static inline ok::StringView view(const FixedString *fs) {
    return {(const char *) fs->items, fs->count};
}

static inline bool operator==(const FixedString &lhs, const ok::StringView &rhs) {
    return view(&lhs) == rhs;
}
} // namespace xmdb
