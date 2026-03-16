#pragma once

#include "ok.hpp"
#include "new.hpp"
#include "DBValue.hpp"

namespace xmdb {
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
            WRITE_COLUMN,
            CALL,
            INSERT,
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

        explicit AlterUserNode(ok::StringView user_name, Property property, DBValue *property_value) : Node{Type::ALTER_USER}, m_user_name{user_name}, m_property{property}, m_property_value{property_value} {}

        ok::StringView user_name() const {
            return m_user_name;
        }

        Property property() const {
            return m_property;
        }

        DBValue *property_value() {
            return m_property_value;
        }

    private:
        ok::StringView m_user_name;
        Property m_property;
        DBValue *m_property_value;
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

    class WriteColumnNode : public Node {
    public:
        explicit WriteColumnNode(DBTable *table, UZ idx, DBValue *value) : Node{Type::WRITE_COLUMN},
                                                                           m_table{table},
                                                                           m_idx{idx},
                                                                           m_value{value} {
        }

        DBTable *table() {
            return m_table;
        }

        UZ idx() {
            return m_idx;
        }

        DBValue *value() {
            return m_value;
        }

    private:
        DBTable *m_table;
        UZ m_idx;
        DBValue *m_value;
    };

    class CallNode : public Node {
    public:
        explicit CallNode(ok::Allocator *allocator,
                          ok::StringView fn_name,
                          Slice<DBValue *> args) : Node{Type::CALL},
                                                   m_fn_name{fn_name},
                                                   m_args{args},
                                                   m_return_value{new (allocator) DelayedDBValue{}} {
        }

        DBValue *return_value() {
            return m_return_value;
        }

    private:
        ok::StringView m_fn_name;
        Slice<DBValue *> m_args;
        DelayedDBValue *m_return_value;
    };

    class InsertNode : public Node {
    public:
        using ValuesSlice = Slice<ok::Pair<StringView, DBValue *>>;

        explicit InsertNode(DBTable *table,
                            Slice<StringView> columns_names,
                            Slice<DBValue *> columns_values,
                            UZ rows_count) : Node{Type::INSERT},
                                             m_table{table},
                                             m_columns_names{columns_names},
                                             m_columns_values{columns_values},
                                             m_rows_count{rows_count} {
        }

    private:
        DBTable *m_table;
        Slice<StringView> m_columns_names;
        Slice<DBValue *> m_columns_values;
        UZ m_rows_count;
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

    WriteColumnNode *write_column(DBTable *table, UZ idx, DBValue *value) {
        return add_generic_node<WriteColumnNode>(table, idx, value);
    }

    CallNode *call(ok::Allocator *allocator, ok::StringView fn_name, Slice<DBValue *> args) {
        return add_generic_node<CallNode>(allocator, fn_name, args);
    }

    InsertNode *insert(DBTable *table,
                       Slice<StringView> names,
                       Slice<DBValue *> values,
                       UZ rows_count) {
        return add_generic_node<InsertNode>(table, names, values, rows_count);
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
