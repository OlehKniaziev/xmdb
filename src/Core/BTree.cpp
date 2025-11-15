#include "BTree.hpp"

namespace xmdb {
constexpr U64 BTREE_PAGE_SIZE = 4096;

constexpr U16 BTREE_HEADER_VERSION = 1;
constexpr U64 BTREE_HEADER_MAGIC = ((U64) 'x' << 56) | ((U64) 'm' << 48) | ((U64) 'd' << 40) | ((U64) 'b' << 32);
constexpr U64 BTREE_INDEX_HEADER_LENGTH = BTREE_PAGE_SIZE;

constexpr U64 BTREE_ORDER = 128;

static_assert(BTREE_ORDER >= 2, "Order of the B-Tree is not allowed to be less than 2");

struct BTreeState;

constexpr auto BTREE_MAX_KEYS = BTREE_ORDER * 2 - 1;
constexpr auto BTREE_MAX_CHILDREN = BTREE_ORDER * 2;

struct DiskHeader {
    static DiskHeader *alloc(ok::Allocator *allocator) {
        auto *header = allocator->alloc<DiskHeader>();
        header->magic = BTREE_HEADER_MAGIC;
        header->version = BTREE_HEADER_VERSION;
        header->index_nodes_count = 0;
        header->payload_length = 0;
        return header;
    }

    U64 magic;
    U16 version;
    U64 index_nodes_count;
    U64 payload_length;
};

struct DiskNode {
    bool is_leaf : 1;
    // NOTE(oleh): Not sure if this is needed at all.
    bool is_root : 1;
    U64 keys_count : 62;

    U64 keys[BTREE_MAX_KEYS];
    U64 children[BTREE_MAX_CHILDREN];
};

static_assert(sizeof(DiskNode) == BTREE_PAGE_SIZE);

struct Node {
    static Node *alloc(ok::Allocator *allocator) {
        auto *node = allocator->alloc<Node>();
        node->disk = allocator->alloc<DiskNode>();
        node->disk->keys_count = 0;
        node->disk->is_leaf = false;
        node->disk->is_root = false;
        return node;
    }

    inline bool is_full() {
        return disk->keys_count == BTREE_MAX_KEYS;
    }

    inline bool is_leaf() {
        return disk->is_leaf;
    }

    inline bool is_root() {
        return disk->is_root;
    }

    inline void set_leaf(bool v) {
        disk->is_leaf = v;
    }

    inline void set_root(bool v) {
        disk->is_root = v;
    }

    inline ok::Slice<U64> keys() {
        return {disk->keys, disk->keys_count};
    }

    inline ok::Slice<U64> children() {
        if (is_leaf()) return {};
        return {disk->children, disk->keys_count + 1};
    }

    inline U64 keys_count() {
        return disk->keys_count;
    }

    inline void add_keys_count(U64 n) {
        disk->keys_count += n;
    }

    inline void set_keys_count(U64 n) {
        disk->keys_count = n;
    }

    DiskNode *disk;
    U64 id;
};

struct BTreeState {
    static ok::Optional<ok::String> create(ok::Allocator *allocator, ok::File state_file, BTreeState **out_state) {
        BTreeState *state = nullptr;

        if (state_file.size() == 0) {
            state = allocator->alloc<BTreeState>();
            state->header = DiskHeader::alloc(allocator);
            state->allocator = allocator;
            state->state_file = state_file;
            state->root = Node::alloc(allocator);
            state->root->set_leaf(true);
            state->root->set_root(true);
            state->root->id = 0;

            UZ temp_buf_count = BTREE_INDEX_HEADER_LENGTH + BTREE_PAGE_SIZE;
            U8 *temp_buf = ok::temp_allocator->alloc<U8>(temp_buf_count);

            memcpy(temp_buf, state->header, sizeof(*state->header));
            memcpy(temp_buf + BTREE_INDEX_HEADER_LENGTH, state->root->disk, BTREE_PAGE_SIZE);

            auto write_err = state_file.write(temp_buf, temp_buf_count);
            if (write_err) return ok::File::error_string(allocator, write_err.value);
        } else {
            // NOTE(oleh): Read in the header and root.
            constexpr auto buffer_count = BTREE_INDEX_HEADER_LENGTH + BTREE_PAGE_SIZE;
            auto *buffer = allocator->alloc<U8>(buffer_count);

            UZ read_count = 0;
            auto read_err = state_file.read(buffer, buffer_count, &read_count);
            if (read_err) return ok::File::error_string(allocator, read_err.value);

            OK_VERIFY(read_count == buffer_count);

            auto *state = allocator->alloc<BTreeState>();
            state->allocator = allocator;
            state->header = (DiskHeader *) buffer;
            state->root->disk = (DiskNode *) (buffer + BTREE_INDEX_HEADER_LENGTH);
            state->root->id = 0;
        }

        *out_state = state;
        return {};
    }

    void free() const {
        OK_TODO();
    }

    S64 node_search(Node *node, U64 key, Node **out_node) {
        if (node == nullptr) {
            return -1;
        }

        U64 key_index;
        for (key_index = 0; key_index < node->keys_count(); key_index++) {
            if (node->keys()[key_index] >= key) break;
        }

        if (key_index < node->keys_count() && node->keys()[key_index] == key) {
            *out_node = node;
            return (S64) key_index;
        }

        if (node->is_leaf()) {
            return -1;
        }

        Node *child = load_node_from_disk(node, key_index);
        OK_ASSERT(child != nullptr);

        return node_search(child, key, out_node);
    }

    inline S64 search(U64 key, Node **out_node) {
        return node_search(root, key, out_node);
    }

    Node *load_node_from_disk(Node *parent, U64 child_index) {
        OK_UNUSED(parent);
        OK_UNUSED(child_index);
        OK_TODO();
    }

    void save_node_to_disk(Node *node) {
        OK_UNUSED(node);
        OK_TODO();
    }

    void split_child(Node *parent, Node *full_child) {
        OK_ASSERT(!parent->is_full());
        OK_ASSERT(full_child->is_full());

        U64 full_child_index = full_child->id;

        Node *new_child = Node::alloc(allocator);
        new_child->set_leaf(full_child->is_leaf());
        new_child->set_keys_count(BTREE_ORDER - 1);

        for (U64 i = 0; i < BTREE_ORDER - 1; ++i) {
            new_child->keys()[i] = full_child->keys()[BTREE_ORDER + i];
        }

        if (!full_child->is_leaf()) {
            for (U64 i = 0; i < BTREE_ORDER; ++i) {
                new_child->children()[i] = full_child->children()[BTREE_ORDER + i];
            }
        }

        full_child->set_keys_count(BTREE_ORDER - 1);

        // NOTE(oleh): Access of children here cannot be out-of-bounds, since the parent node
        // is not full.
        for (S64 i = (S64) parent->keys_count(); i > (S64) full_child_index; --i) {
            parent->children()[i + 1] = parent->children()[i];
        }

        parent->children()[full_child_index + 1] = new_child->id;

        for (S64 i = (S64) parent->keys_count() - 1; i > (S64) full_child_index - 1; --i) {
            parent->keys()[i + 1] = parent->keys()[i];
        }

        U64 median_key = full_child->keys()[BTREE_ORDER];
        parent->add_keys_count(1);
        parent->keys()[full_child_index] = median_key;

        save_node_to_disk(parent);
        save_node_to_disk(full_child);
        save_node_to_disk(new_child);
    }

    void split_child(Node *parent, U64 child_index) {
        Node *full_child = load_node_from_disk(parent, child_index);

        split_child(parent, full_child);
    }

    void insert(U64 key) {
        auto *node = root;

        if (node->is_full()) {
            Node *new_root = Node::alloc(allocator);

            new_root->set_leaf(false);
            new_root->set_keys_count(0);

            new_root->children()[0] = node->id;
            split_child(new_root, (U64) 0);

            this->root = new_root;

            insert_non_full(new_root, key);
        } else {
            insert_non_full(node, key);
        }
    }

    void insert_non_full(Node *node, U64 key) {
        OK_ASSERT(!node->is_full());

        S64 i = (S64) node->keys_count() - 1;

        if (node->is_leaf()) {
            for (; i >= 0 && key < node->keys()[i]; --i) {
                node->keys()[i + 1] = node->keys()[i];
            }

            node->add_keys_count(1);
            node->keys()[i + 1] = key;

            save_node_to_disk(node);
        } else {
            while (i >= 0 && key < node->keys()[i]) {
                --i;
            }

            ++i;

            Node *child = load_node_from_disk(node, i);

            if (child->is_full()) {
                split_child(node, child);

                if (key > node->keys()[i]) {
                    ++i;
                }
            }

            insert_non_full(child, key);
        }
    }

    ok::Allocator *allocator;
    ok::File state_file;
    DiskHeader *header;
    Node *root;
    ok::Slice<U8> write_buffer;
};

ok::Optional<ok::File::OpenError> BTreeIndex::create(ok::Allocator *allocator, ok::StringView state_filename,
                                                     BTreeIndex *index) {
    ok::File state_file{};
    auto open_error = ok::File::open(&state_file, state_filename);
    if (open_error) return open_error;

    *index = BTreeIndex::create(allocator, state_file);
    return {};
}

BTreeIndex BTreeIndex::create(ok::Allocator *allocator, ok::File state_file) {
    BTreeIndex tree{};
    auto error_string = BTreeState::create(allocator, state_file, (BTreeState **)&tree.pImpl);
    OK_VERIFY(!error_string.has_value());
    return tree;
}

void BTreeIndex::insert(U64 key) {
    auto *impl = static_cast<BTreeState *>(pImpl);
    impl->insert(key);
}

bool BTreeIndex::contains(U64 key) {
    auto *impl = static_cast<BTreeState *>(pImpl);
    Node *out_node;
    S64 index = impl->search(key, &out_node);
    return index != -1;
}
}; // namespace xmdb
