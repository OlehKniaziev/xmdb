#pragma once

#include "ok.hpp"

namespace xmdb {
struct FixedString {
    static constexpr UZ DATA_SIZE = 63;
    static constexpr UZ PREFIX_SIZE = 1;

    ok::StringView view() const {
        return {(const char *) items, count};
    }

    U8 count;
    U8 items[DATA_SIZE];
};

static_assert(sizeof(FixedString::count) == FixedString::PREFIX_SIZE);
static_assert(sizeof(FixedString) == FixedString::PREFIX_SIZE + FixedString::DATA_SIZE);

FixedString create_fixed_string(ok::StringView);

static inline bool operator==(const FixedString &lhs, const ok::StringView &rhs) {
    return lhs.view() == rhs;
}
} // namespace xmdb
