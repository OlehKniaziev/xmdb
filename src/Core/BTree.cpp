#include "BTree.hpp"

namespace xmdb {
static BTreeNode *alloc_node(BTreeNodePool *pool, U64 index) {
    BTreeNode *node;

    if (pool->head != nullptr) {
        node = pool->head;
        pool->head = node->next;
        goto ret;
    }

    pool->head = pool->arena.alloc<BTreeNode>();

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
    if (node_read_error)
        OK_PANIC_FMT("Failed to load node from disk: %s", ok::File::error_string(node_read_error.value));

    return new_node;
}

static void btree_store_and_unload_node(BTree *tree, BTreeNode *node) {
    UZ node_offset_in_file = get_node_offset(node->index);
    U8 *node_payload_ptr = (U8 *)&node->payload;
    UZ node_payload_size = sizeof(node->payload);
    tree->file.seek_to(node_offset_in_file);
    ok::Optional<ok::File::WriteError> node_write_error = tree->file.write(node_payload_ptr,
                                                                           node_payload_size);
    if (node_write_error)
        OK_PANIC_FMT("Failed to store node to disk: %s", ok::File::error_string(node_write_error.value));

    free_node(&tree->node_pool, node);
}

// TODO: Update the tree on disk.
static U64 gen_new_index(BTree *tree) {
    return tree->header.node_count++;
}

static void btree_split_child(BTree *tree, BTreeNode *current_node, U64 child_index) {
    U64 new_node_index = gen_new_index(tree);
    BTreeNode *new_node = alloc_node(&tree->node_pool, new_node_index);
    new_node->set_count(BTREE_ORDER - 1);

    BTreeNode *child_node = btree_load_node(tree, child_index);

    for (U64 i = 0; i < BTREE_ORDER - 1; ++i) {
        key_at(new_node, i) = key_at(child_node, i + 1);
    }

    if (!is_leaf(child_node)) {
        for (U64 i = 0; i < BTREE_ORDER; ++i) {
            child_at(new_node, i) = child_at(child_node, i + 1);
        }
    }

    child_node->set_count(BTREE_ORDER);

    for (U64 j = current_node->count() - 1; j > child_index; --j) {
        child_at(current_node, j + 1) = child_at(current_node, j);
    }

    child_at(current_node, child_index + 1) = new_node->index;

    for (U64 j = current_node->count(); j > child_index; --j) {
        key_at(current_node, j + 1) = key_at(current_node, j);
    }

    key_at(current_node, child_index) = key_at(child_node, BTREE_ORDER);
    U64 new_current_node_count = current_node->count() + 1;
    current_node->set_count(new_current_node_count);

    btree_store_and_unload_node(tree, current_node);
    btree_store_and_unload_node(tree, child_node);
    btree_store_and_unload_node(tree, new_node);
}

static bool btree_insert_nonfull(BTree *tree, BTreeNode *node, U64 key_to_insert) {
    OK_ASSERT(node->count() < BTREE_MAX_KEYS);

    S64 i = node->count();
    node->set_count(i + 1);

    if (node->is_leaf()) {
        for (; i >= 0 && key_to_insert < key_at(node, i); --i) {
            key_at(node, i + 1) = key_at(node, i);
        }

        key_at(node, i + 1) = key_to_insert;
        btree_store_and_unload_node(tree, node);
    } else {
        for (; i >= 0 && key_to_insert < key_at(node, i); --i);
        i += 1;

        U64 child_node_idx = child_at(node, i);
        // is this even right??
        BTreeNode *child_node = btree_load_node(tree, child_node_idx);

        if (child_node->count() == BTREE_MAX_KEYS) {
            btree_split_child(tree, node, child_node_idx);

            U64 insertion_position_key = key_at(node, i);
            if (key_to_insert > insertion_position_key) i += 1;
        }

        btree_insert_nonfull(tree, child_node, key_to_insert);
    }
}

static bool btree_insert(BTree *tree, U64 key) {
    BTreeNode *root = tree->root_node;
    if (root->count == BTREE_MAX_KEYS) {
        BTreeNode *new_root = alloc_node(&tree->node_pool);
        tree->root_node = new_root;

        new_root->is_leaf = false;
        new_root->count = 0;
        child_at(new_root, 0) = root;
        btree_split_child(tree, new_root, 0);
        return btree_insert_nonfull(tree, new_root, key);
    } else {
        return btree_insert_nonfull(tree, root, key);
    }
}

static bool btree_search_node(BTree *tree,
                              U64 key,
                              BTreeNode **out_node,
                              U64 *out_index) {
    BTreeNode *node = tree->current_node;
    U64 node_count = tree->current_node_count;

    U64 i = 0;

    while (i < node_count && key > node->keys[i]) ++i;

    if (i < node_count && node->keys[i] == key) {
        *out_node = node;
        *out_index = i;
        return true;
    }

    if (is_leaf(node)) return false;

    btree_load_node(tree, node->children[i]);

    return btree_search_node(tree, key, out_node, out_index);
}

static constexpr U32 BTREE_FORMAT_VERSION = 0x00'00'00'00;

BTree btree_create(ok::File file) {
    BTree tree;
    tree.file = file;
    tree.version = BTREE_FORMAT_VERSION;
    tree.node_pool = new_node_pool();
    tree.root_node = alloc_node(&tree.node_pool);
    btree_store_node(&tree, tree.root_node);
    return tree;
}

bool btree_search(BTree *tree, U64 key, BTreeNode **out_node, U64 *out_index) {
    tree->current_node = tree->root_node;
    tree->current_node_count = tree->root_node_count;
    return btree_search_node(tree, key, out_node, out_index);
}
};
