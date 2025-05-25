#ifndef XMDB_RBTREE_HPP
#define XMDB_RBTREE_HPP

#include "ok.hpp"

туду нахуй

template <typename K, typename V>
struct RBTreeMap {
    struct Node {
        enum {
            RED,
            BLACK,
        };

        static constexpr U32 NIL_NODE = (U32)-1;

        int color;
        K key;
        V value;
        U32 left_child = NIL_NODE;
        U32 right_child = NIL_NODE;
    };

    Optional<V*> search(const K& key);

    ok::Allocator* allocator;
    ok::List<Node> nodes;
    U32 root_node_index;
};

template <typename K, typename V>
Optional<V*> RBTreeMap::search(const K& key) {
    Node* node = &nodes[root_node_index];

    while (true) {
        if (node->key == key) return &node->value;

        if (key < node->key) {
            U32 left_child = node->left_child;
            if (left_child == Node::NIL_NODE) break;
            node = &nodes[left_child];
        } else {
            U32 right_child = node->right_child;
            if (right_child == Node::NIL_NODE) break;
            node = &nodes[right_child];
        }
    }

    return {};
}

#endif // XMDB_RBTREE_HPP
