#pragma once

#include "ok.hpp"
#include "new.hpp"

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
            COMPARE,
            CONSTANT,
            ALTER_USER,
            ATOMIC,
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

        ok::Slice<const U8> buffer() const {
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

    class CompareNode : public Node {
    public:
        CompareNode(const Node *lhs, const Node *rhs) : Node{Type::COMPARE}, m_lhs{lhs}, m_rhs{rhs} {}

    private:
        const Node *m_lhs;
        const Node *m_rhs;
    };

    class ConstantNode : public Node {
    public:
        explicit ConstantNode(bool b) : Node{Type::CONSTANT}, m_kind{Kind::BOOL}, m_u{.b = b} {}
        explicit ConstantNode(S64 i) : Node{Type::CONSTANT}, m_kind{Kind::INT}, m_u{.i = i} {}
        explicit ConstantNode(ok::StringView s) : Node{Type::CONSTANT}, m_kind{Kind::STRING}, m_u{.s = s} {}

        enum class Kind {
            BOOL,
            INT,
            STRING,
        };

    private:
        Kind m_kind;
        union {
            bool b;
            S64 i;
            ok::StringView s;
        } m_u;
    };

    class AlterUserNode : public Node {
    public:
        enum class Property {
            PASSWORD,
        };

        explicit AlterUserNode(ok::StringView user_name, Property property, const Node *property_value) : Node{Type::ALTER_USER}, m_user_name{user_name}, m_property{property}, m_property_value{property_value} {}

    private:
        ok::StringView m_user_name;
        Property m_property;
        const Node *m_property_value;
    };

    class AtomicNode : public Node {
    public:
        explicit AtomicNode(ok::Allocator *allocator) : Node{Type::ATOMIC} {
            m_nodes = ok::List<Node *>::alloc(allocator);
        }

        void add(Node *node) {
            m_nodes.push(node);
        }

    private:
        ok::List<Node *> m_nodes;
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

    CompareNode *compare(const Node *lhs, const Node *rhs) {
        return add_generic_node<CompareNode>(lhs, rhs);
    }

    ConstantNode *const_bool(bool b) {
        return add_generic_node<ConstantNode>(b);
    }

    ConstantNode *const_int(S64 i) {
        return add_generic_node<ConstantNode>(i);
    }

    ConstantNode *const_string(ok::StringView s) {
        return add_generic_node<ConstantNode>(s);
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
