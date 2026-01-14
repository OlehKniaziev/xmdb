#pragma once

#include "ok.hpp"
#include "new.hpp"
#include "DBValue.hpp"

namespace xmdb {
// Nodes in this graph represent an action performed on either a record in a table or another node.
//
// For example:
// * ReadNode(offset, size) - reads [size] bytes from the record's data at offset [offset]
// * CompareNode(lhs, rhs) - compares nodes [lhs] and [rhs], and serves as an input to another node
class QueryGraph {
public:
    explicit QueryGraph(ok::Allocator *allocator) : m_allocator{allocator} {}

    class Node {
    public:
        Node() = delete;

        enum class Type {
            READ,
            WRITE,
            ALTER_USER,
            ATOMIC,
            VALUE,
        };

        Type type() const {
            return m_type;
        }

        ok::Optional<Node *> next() {
            return m_next;
        }

        void set_next(Node *node) {
            m_next = node;
        }

    protected:
        Node(Type type) : m_type{type} {
        }

        ok::Optional<Node *> m_next{};
        Type m_type;
    };

    class ReadNode : public Node {
    public:
        ReadNode(U64 offset, U64 size) : Node{Type::READ}, m_size{size}, m_offset{offset} {}

        U64 size() const {
            return m_size;
        }

        U64 offset() const {
            return m_offset;
        }

        ok::Slice<U8> buffer() const {
            return m_buffer;
        }

        void set_buffer(ok::Slice<U8> buffer) {
            m_buffer = buffer;
        }

    private:
        U64 m_size;
        U64 m_offset;
        ok::Slice<U8> m_buffer;
    };

    class WriteNode : public Node {
    public:
        WriteNode(U64 offset, U64 size) : Node{Type::WRITE}, m_size{size}, m_offset{offset} {}

        U64 size() const {
            return m_size;
        }

        U64 offset() const {
            return m_offset;
        }

    private:
        U64 m_size;
        U64 m_offset;
    };

    class AlterUserNode : public Node {
    public:
        enum class Property {
            PASSWORD,
        };

        explicit AlterUserNode(ok::StringView user_name, Property property, Node *property_value) : Node{Type::ALTER_USER}, m_user_name{user_name}, m_property{property}, m_property_value{property_value} {}

        ok::StringView user_name() const {
            return m_user_name;
        }

        Property property() const {
            return m_property;
        }

        Node *property_value() {
            return m_property_value;
        }

    private:
        ok::StringView m_user_name;
        Property m_property;
        Node *m_property_value;
    };

    class AtomicNode : public Node {
    public:
        explicit AtomicNode(ok::Allocator *allocator) : Node{Type::ATOMIC} {
            m_nodes = ok::List<Node *>::alloc(allocator);
        }

        void add(Node *node) {
            m_nodes.push(node);
        }

        ok::Slice<Node *> nodes() {
            return m_nodes.slice();
        }

    private:
        ok::List<Node *> m_nodes;
    };

    class ValueNode : public Node {
    public:
        explicit ValueNode(DBValue *value) : Node{Type::VALUE}, m_value{value} {}

        DBValue *value() {
            return m_value;
        }

    private:
        DBValue *m_value;
    };

    ok::Optional<Node *> root_node() const {
        return m_root_node;
    }

    ReadNode *read(U64 offset, U64 size) {
        return add_generic_node<ReadNode>(offset, size);
    }

    WriteNode *write(U64 offset, U64 size) {
        return add_generic_node<WriteNode>(offset, size);
    }

    AtomicNode *atomic() {
        return add_generic_node<AtomicNode>(m_allocator);
    }

    void reset();

private:
    template <typename T, typename... Args>
    T *add_generic_node(Args... args) {
        T *node = new (m_allocator) T{args...};

        if (!m_last_node) {
            m_root_node = node;
            m_last_node = node;
            return node;
        }

        Node *last = m_last_node.get();
        last->set_next(node);
        m_last_node = node;
        return node;
    }

private:
    ok::Allocator *m_allocator;
    ok::Optional<Node *> m_root_node;
    ok::Optional<Node *> m_last_node;
};
} // namespace xmdb
