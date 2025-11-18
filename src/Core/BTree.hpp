#pragma once

#include "ok.hpp"

namespace xmdb {
constexpr U64 BTREE_PAGE_SIZE = 4096;

constexpr U16 BTREE_HEADER_VERSION = 1;
constexpr U64 BTREE_HEADER_MAGIC = ((U64) 'x' << 56) | ((U64) 'm' << 48) | ((U64) 'd' << 40) | ((U64) 'b' << 32);
constexpr U64 BTREE_HEADER_LENGTH = BTREE_PAGE_SIZE;

constexpr U64 BTREE_ORDER = 128;

static_assert(BTREE_ORDER >= 2, "Order of the B-Tree is not allowed to be less than 2");

constexpr auto BTREE_MAX_KEYS = BTREE_ORDER * 2 - 1;
constexpr auto BTREE_MAX_CHILDREN = BTREE_ORDER * 2;

struct BTreeIndex {
    static ok::Optional<ok::File::OpenError> create(ok::Allocator *allocator, ok::StringView state_filename,
                                                    BTreeIndex *index);
    static BTreeIndex create(ok::Allocator *allocator, ok::File state_file);

    void insert(U64);

    bool contains(U64);

    bool first_constructed();

    UZ order();

    U64 height();

    U64 node_count();

    void *pImpl;
};
}; // namespace xmdb
