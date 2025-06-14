#include "BTree.hpp"

namespace xmdb {
static BTreeNode *alloc_node(BTreeNodePool *pool, U64 index) {
    BTreeNode *node;

    if (pool->head != nullptr) {
        node = pool->head;
        pool->head = node->next;
        goto ret;
    }

    pool->head = pool->arena->alloc<BTreeNode>();

ret:
    node->index = index;
    return pool->head;
}

static void free_node(BTreeNodePool *pool, BTreeNode *node) {
    node->next = pool->head;
    pool->head = node;
}

static constexpr UZ BTREE_HEADER_SIZE = sizeof(BTreeHeader);

static inline UZ get_node_offset(U64 index) {
    return BTREE_HEADER_SIZE + index * BTREE_PAGE_SIZE;
}

static inline bool is_leaf(BTreeNode *node) {
    return node->payload.meta & (1 << (sizeof(node->payload.meta) - 1)) >> (sizeof(node->payload.meta) - 1);
}

static inline U64 &key_at(BTreeNode *node, U64 i) {
    OK_ASSERT(i < node->count());
    return node->payload.keys[i];
}

static inline U64 &child_at(BTreeNode *node, U64 i) {
    OK_ASSERT(!is_leaf(node));
    OK_ASSERT(i < node->count());
    return node->payload.children[i];
}

static BTreeNode *btree_load_node(BTree *tree, U64 index) {
    BTreeNode *new_node = alloc_node(&tree->node_pool, index);

    UZ node_offset_in_file = get_node_offset(index);
    U8 *new_node_payload_ptr = (U8 *)&new_node->payload;
    UZ new_node_payload_size = sizeof(new_node->payload);
    tree->file.seek_to(node_offset_in_file);
    ok::Optional<ok::File::ReadError> node_read_error = tree->file.read(new_node_payload_ptr,
                                                                        new_node_payload_size,
                                                                        nullptr);
    if (node_read_error) {
        ok::String error_string = ok::File::error_string(ok::temp_allocator, node_read_error.value);
        OK_PANIC_FMT("Failed to load node from disk: %s", error_string.cstr());
    }

    return new_node;
}

static void btree_store_node(BTree *tree, BTreeNode *node) {
    UZ node_offset_in_file = get_node_offset(node->index);
    U8 *node_payload_ptr = (U8 *)&node->payload;
    UZ node_payload_size = sizeof(node->payload);
    tree->file.seek_to(node_offset_in_file);
    ok::Optional<ok::File::WriteError> node_write_error = tree->file.write(node_payload_ptr,
                                                                           node_payload_size);
    if (node_write_error) {
        ok::String error_string = ok::File::error_string(ok::temp_allocator, node_write_error.value);
        OK_PANIC_FMT("Failed to store node to disk: %s", error_string.cstr());
    }
}

static void btree_store_and_unload_node(BTree *tree, BTreeNode *node) {
    btree_store_node(tree, node);
    free_node(&tree->node_pool, node);
}

// TODO: Update the tree on disk.
static U64 gen_new_index(BTree *tree) {
    return tree->header.node_count++;
}

static constexpr U32 BTREE_FORMAT_VERSION = 0x00'00'00'00;

bool search_node(BTree *tree,
                 BTreeNode *node,
                 BTreeKeyType key,
                 BTreeNode **out_node,
                 U16 *out_index) {
    U64 node_count = node->count();

    U16 i = 0;
    while(i < node_count && key > node->key_at(i)) {
        ++i;
    }

    if (i < node_count && key == node->key_at(i)) {
        *out_node = node;
        *out_index = i;
        return true;
    }

    if (node->is_leaf()) return false;

    BTreeChildType child = node->child_at(i);
    BTreeNode *child_node = btree_load_node(tree, child);
}

BTree::BTree(ok::File file, ok::ArenaAllocator *arena) : file{file}, node_pool{arena} {
    header.version = BTREE_FORMAT_VERSION;
    header.node_count = 1;

    root_node = alloc_node(&node_pool, 0);
    root_node->set_is_leaf(true);
    btree_store_node(this, root_node);
}

bool BTree::search(BTreeKeyType key, BTreeNode **out_node, U16 *out_index) {
    return search_node(this, this->root_node, key, out_node, out_index);
}
};
