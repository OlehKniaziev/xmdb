#ifndef XMDB_BTREE_H_
#define XMDB_BTREE_H_

#include "ok.hpp"

using ok::Allocator;
using ok::List;

namespace xmdb {
static constexpr U16 BTREE_PAGE_SIZE = 4096;
static constexpr U16 BTREE_ORDER = BTREE_PAGE_SIZE / 32;
static constexpr U16 BTREE_MAX_KEYS = BTREE_ORDER * 2 - 1;
static constexpr U16 BTREE_MAX_CHILDREN = BTREE_ORDER * 2;

static_assert((BTREE_MAX_KEYS + BTREE_MAX_CHILDREN + 1) * 8 == BTREE_PAGE_SIZE);

using BTreeMetaType = U64;
using BTreeKeyType = U64;
using BTreeChildType = U64;

/*
 * Disk layout of the B-Tree node is as following:
 *   64-bit meta field
 *   BTREE_MAX_KEYS 64-bit keys
 *   BTREE_MAX_CHILDREN 64-bit children indices
 */

struct BTreeNodePayload {
    /*
      0th bit   - 1 if leaf, 0 if internal / root
      1-63 bits - the count of keys
     */
    BTreeMetaType meta;
    BTreeKeyType keys[BTREE_MAX_KEYS];
    BTreeChildType children[BTREE_MAX_CHILDREN];
};

static_assert(sizeof(BTreeNodePayload) == BTREE_PAGE_SIZE);

struct BTreeNode {
    BTreeNode *next;
    BTreeNodePayload payload;
    U64 index;

    inline U64 count() const {
        return payload.meta & ~((U64)1 << 63);
    }

    inline void set_count(U64 count) {
        if (payload.meta >= UINT64_MAX - count) OK_PANIC("node reached max count");
        payload.meta += count;
    }

    inline bool is_leaf() const {
        return (payload.meta & ((U64)1 << 63)) >> 63;
    }

    inline void set_is_leaf(bool bool_value) {
        U64 value = (U64)bool_value;
        payload.meta |= value << 63;
    }
};

struct BTreeNodePool {
    explicit BTreeNodePool(ok::ArenaAllocator *arena) : arena{arena}, head{nullptr} {}

    ok::ArenaAllocator *arena;
    BTreeNode *head;
};

/*
 * Disk layout of the B-Tree is as following:
 *   0-4   - version of the tree structure
 *   4-8   - node count
 *   8-end - nodes
 */

struct BTreeHeader {
    U32 version;
    U32 node_count;
};

struct BTree {
    ok::File file;
    BTreeNodePool node_pool;
    BTreeHeader header;
    BTreeNode *root_node;

    explicit BTree(ok::File file, ok::ArenaAllocator *arena);

    bool search(BTreeKeyType key, BTreeNode **out_node, U16 *out_index);
    bool insert(BTreeKeyType key);
};
}; // namespace xmdb

#endif // XMDB_BTREE_H_
