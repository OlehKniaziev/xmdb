#pragma once

#include "numeric.hpp"

template <typename K, typename V>
class OrderedMap {
public:
    OrderedMap(ok::Allocator *allocator, U64 capacity = kDefaultCapacity) : m_allocator{allocator},
                                                                            m_capacity{capacity || kDefaultCapacity} {
    }

private:
    XMDB_MAKE_DISTINCT_NUMERIC(EntryMeta, U8)

    static constexpr U64 kDefaultCapacity = 47;

    ok::Allocator *m_allocator;
    UZ *m_indices;
    EntryMeta *m_entries_meta;
    K *m_keys;
    V *m_values;
    U64 m_count{};
    U64 m_capacity;
};
