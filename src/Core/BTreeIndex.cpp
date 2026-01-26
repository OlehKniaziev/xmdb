// TODO(oleh): Make this load nodes only if they aren't in memory already.
// This would require an in-memory index of live nodes, or something like that.
#include "BTreeIndex.hpp"
#include "Logger.hpp"
#include "constants.hpp"

namespace xmdb {
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
    U64 height;
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
    static Node *alloc_without_backing_disk_buffer(ok::Allocator *allocator) {
        auto *node = allocator->alloc<Node>();
        return node;
    }

    static Node *alloc_with_backing_disk_buffer(ok::Allocator *allocator) {
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
        return {disk->children, children_count()};
    }

    inline U64 keys_count() {
        return disk->keys_count;
    }

    inline U64 children_count() {
        return keys_count() + 1;
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

using BTreeStateFlags = U16;

enum {
    FLAG_FIRST_CONSTRUCTION = 1 << 0,
};

struct BTreeState {
    static ok::Optional<ok::String> create(ok::Allocator *allocator, ok::File state_file, BTreeState **out_state) {
        BTreeState *state = nullptr;

        state_file.seek_start();

        if (state_file.size() == 0) {
            state = allocator->alloc<BTreeState>();
            state->allocator = allocator;
            state->state_file = state_file;

            if (auto err = state->reset()) return err;
            *out_state = state;
            return {};
        } else {
            // NOTE(oleh): Read in the header and root.
            constexpr auto buffer_count = BTREE_HEADER_LENGTH + BTREE_PAGE_SIZE;
            auto *buffer = allocator->alloc<U8>(buffer_count);

            UZ read_count = 0;
            auto read_err = state_file.read(buffer, buffer_count, &read_count);
            if (read_err) return ok::File::error_string(allocator, read_err.value);

            OK_VERIFY(read_count == buffer_count);

            state = allocator->alloc<BTreeState>();
            state->flags = 0;
            state->allocator = allocator;
            state->header = (DiskHeader *) buffer;
            state->root = Node::alloc_with_backing_disk_buffer(allocator);
            state->root->disk = (DiskNode *) (buffer + BTREE_HEADER_LENGTH);
            state->root->id = 0;
        }

        *out_state = state;
        return {};
    }

    ok::Optional<ok::String> reset() {
        this->flags = FLAG_FIRST_CONSTRUCTION;
        this->header = DiskHeader::alloc(allocator);
        this->header->height = 1;
        this->header->index_nodes_count = 1;
        this->root = Node::alloc_with_backing_disk_buffer(allocator);
        this->root->set_leaf(true);
        this->root->set_root(true);
        this->root->id = 0;

        UZ temp_buf_count = BTREE_HEADER_LENGTH + BTREE_PAGE_SIZE;
        U8 *temp_buf = ok::temp_allocator()->alloc<U8>(temp_buf_count);

        memcpy(temp_buf, this->header, sizeof(*this->header));
        memcpy(temp_buf + BTREE_HEADER_LENGTH, this->root->disk, BTREE_PAGE_SIZE);

        state_file.seek_start();

        UZ n_written = 0;
        auto write_err = state_file.write(temp_buf, temp_buf_count, &n_written);
        if (write_err) return ok::File::error_string(allocator, write_err.value);
        OK_VERIFY(n_written == temp_buf_count);
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

    inline bool remove_node(Node *node, U64 key) {
#if 0
        S64 i = 0;

        for (; i < (S64)node->keys_count() && key < node->keys()[i]; ++i) {
        }

        if (i == (S64)node->keys_count()) {
            if (node->is_leaf()) return false;
            Node *child = load_node_from_disk(node, i);
            return remove_node(child, key);
        }

        if (node->keys()[i] == key) {
            if (node->is_leaf()) {
                for (; i < (S64)node->keys_count() - 1; ++i) {
                    node->keys()[i] = node->keys()[i - 1];
                }

                return true;
            }
        }
#else
        OK_UNUSED(node);
        OK_UNUSED(key);

        OK_TODO();
#endif // 0
    }

    bool remove(U64 key) {
        return remove_node(root, key);
    }

    Node *load_node_from_disk(Node *parent, U64 child_index) {
        U64 node_id = parent->children()[child_index];

        state_file.seek_to(BTREE_HEADER_LENGTH + node_id * BTREE_PAGE_SIZE);

        UZ num_read = 0;
        Node *node = Node::alloc_with_backing_disk_buffer(allocator);

        auto read_err = state_file.read(reinterpret_cast<U8 *>(node->disk), BTREE_PAGE_SIZE, &num_read);
        if (read_err) OK_TODO();

        OK_VERIFY(num_read == BTREE_PAGE_SIZE);

        node->id = node_id;

        return node;
    }

    void save_node_to_disk(Node *node) {
        state_file.seek_to(BTREE_HEADER_LENGTH + node->id * BTREE_PAGE_SIZE);

        UZ n_written = 0;
        auto write_err = state_file.write(reinterpret_cast<U8 *>(node->disk), BTREE_PAGE_SIZE, &n_written);
        if (write_err) OK_TODO();
        OK_VERIFY(n_written == BTREE_PAGE_SIZE);
    }

    void split_child(Node *parent, Node *full_child, U64 full_child_index) {
        OK_ASSERT(!parent->is_full());
        OK_ASSERT(full_child->is_full());

        U64 full_child_id = full_child->id;

        U64 median_key = full_child->keys()[BTREE_ORDER - 1];

        Node *new_child = alloc_node_with_backing_disk_buffer();
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
        for (S64 i = (S64) parent->keys_count(); i > (S64) full_child_id; --i) {
            parent->children()[i + 1] = parent->children()[i];
        }

        for (S64 i = (S64) parent->keys_count() - 1; i > (S64) full_child_id - 1; --i) {
            parent->keys()[i + 1] = parent->keys()[i];
        }

        parent->add_keys_count(1);
        parent->children()[full_child_index + 1] = new_child->id;

        parent->keys()[full_child_index] = median_key;

        save_node_to_disk(parent);
        save_node_to_disk(full_child);
        save_node_to_disk(new_child);
    }

    void insert(U64 key) {
        if (root->is_full()) {
            Node *new_root = alloc_node_with_backing_disk_buffer();

            new_root->set_leaf(false);
            new_root->set_keys_count(0);

            root->id = new_root->id;
            new_root->id = 0;

            new_root->children()[0] = root->id;
            split_child(new_root, root, 0);

            this->root = new_root;

            ++header->height;

            insert_non_full(new_root, key);
        } else {
            insert_non_full(root, key);
        }
    }

    void insert_non_full(Node *node, U64 key) {
        OK_ASSERT(!node->is_full());

        S64 i = (S64) node->keys_count() - 1;

        if (node->is_leaf()) {
            node->add_keys_count(1);

            for (; i >= 0 && key < node->keys()[i]; --i) {
                node->keys()[i + 1] = node->keys()[i];
            }

            node->keys()[i + 1] = key;

            save_node_to_disk(node);
        } else {
            while (i >= 0 && key < node->keys()[i]) {
                --i;
            }

            ++i;

            Node *child = load_node_from_disk(node, i);

            if (child->is_full()) {
                split_child(node, child, i);

                if (key > node->keys()[i]) {
                    ++i;
                }
            }

            insert_non_full(child, key);
        }
    }

    Node *alloc_node_without_backing_disk_buffer() {
        ++header->index_nodes_count;
        auto *node = Node::alloc_with_backing_disk_buffer(allocator);
        node->id = ++current_node_id;
        return node;
    }

    Node *alloc_node_with_backing_disk_buffer() {
        ++header->index_nodes_count;
        auto *node = Node::alloc_with_backing_disk_buffer(allocator);
        node->id = ++current_node_id;
        return node;
    }

    ok::Allocator *allocator;
    ok::File state_file;
    DiskHeader *header;
    Node *root;
    ok::Slice<U8> write_buffer;
    BTreeStateFlags flags;
    U64 current_node_id;
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
    auto error_string = BTreeState::create(allocator, state_file, (BTreeState **) &tree.pImpl);
    OK_VERIFY(!error_string.has_value());
    OK_VERIFY(tree.pImpl != nullptr);
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

bool BTreeIndex::remove(U64 key) {
    auto *impl = static_cast<BTreeState *>(pImpl);
    return impl->remove(key);
}

bool BTreeIndex::first_constructed() {
    auto *impl = static_cast<BTreeState *>(pImpl);
    return impl->flags & FLAG_FIRST_CONSTRUCTION;
}

UZ BTreeIndex::order() {
    return BTREE_ORDER;
}

U64 BTreeIndex::height() {
    auto *impl = static_cast<BTreeState *>(pImpl);
    return impl->header->height;
}

U64 BTreeIndex::node_count() {
    auto *impl = static_cast<BTreeState *>(pImpl);
    return impl->header->index_nodes_count;
}

bool BTreeIndex::reset() {
    auto *impl = static_cast<BTreeState *>(pImpl);
    return !impl->reset().has_value();
}
} // namespace xmdb
