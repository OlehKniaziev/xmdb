#pragma once

#include "numeric.hpp"

/**
 * @brief A map that maintains the order of elements as they were inserted.
 * @tparam K The type of keys.
 * @tparam V The type of values.
 */
template <typename K, typename V>
class OrderedMap {
public:
    /**
     * @brief Constructs a new OrderedMap.
     * @param allocator The allocator to use.
     * @param capacity The initial capacity of the map.
     */
    OrderedMap(ok::Allocator *allocator, U64 capacity = kDefaultCapacity);

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
