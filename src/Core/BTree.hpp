#ifndef XMDB_BTREE_H_
#define XMDB_BTREE_H_

#include "ok.hpp"

using ok::Allocator;
using ok::List;

namespace xmdb {
template<typename K, typename V>
struct BTree {
    struct Node {
        static Node alloc(Allocator *alloc, size_t max_keys, bool is_leaf) {
            Node node;
            node.keys = List<K>::alloc(alloc, max_keys);
            node.values = List<V>::alloc(alloc);
            node.children = List<Node>::alloc(alloc);
            node.is_leaf = is_leaf;

            return node;
        }

        bool is_leaf;
        List<K> keys;
        List<V> values;
        List<Node> children;
    };

    static BTree alloc(Allocator *alloc, size_t max_keys_per_node) {
        BTree tree;
        tree.allocator = alloc;
        tree.max_keys_per_node = max_keys_per_node;
        tree.root = Node::alloc(alloc, max_keys_per_node, false /* should this be `true`? */);

        return tree;
    }

    bool search_node(const K &key, Node **out, size_t *idx) {
        Node *node = &root;

        while (true) {
            size_t low = 0;
            size_t high = node->keys.count - 1;

            size_t median;

            while (low <= high) {
                median = (low + high) / 2;

                const K &median_key = node->keys[median];

                if (median_key == key) {
                    *out = node;
                    *idx = median;
                    return true;
                }

                if (median_key < key)
                    low = median + 1;
                else
                    high = median - 1;
            }

            if (node->is_leaf) {
                *out = node;
                *idx = low;
                return false;
            }

            node = &node->children[median];
        }
    }

    void insert(const K &key, const V &value) {
        Node *node;
        size_t idx;

        if (search_node(key, &node, &idx)) {
            node->values[idx] = value;
            return;
        }

        // TODO: check if the tree needs to be re-balanced
        if (node->keys.count < max_keys_per_node) {
            node->keys.insert(idx, value);
            return;
        }

        size_t median = node->keys.count / 2;

        Node left = Node::alloc(allocator);
        Node right = Node::alloc(allocator);

        for (size_t i = 0; i < median; i++)
            left.keys.push(node->keys[i]);
        for (size_t i = median; i < node->keys.count; i++)
            right.keys.push(node->keys[i]);
    }

    Allocator *allocator;
    size_t max_keys_per_node;
    Node root;
};
}; // namespace xmdb

#endif // XMDB_BTREE_H_
