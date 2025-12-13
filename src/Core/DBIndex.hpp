#pragma once

#include "ok.hpp"

#include "DBRecord.hpp"

namespace xmdb {
template <typename TKey, typename TValue>
struct TypedDBIndex {
    ok::Table<TKey, TValue> values;
};

union DBIndex {
    template <typename K>
    using Index = TypedDBIndex<K, DBRecord>;

    Index<ok::StringView> string;
    Index<U64> integer;
};
};
