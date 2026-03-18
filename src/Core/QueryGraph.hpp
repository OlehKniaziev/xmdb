#pragma once

#include "ok.hpp"
#include "new.hpp"
#include "DBValue.hpp"
#include "DBTable.hpp"

namespace xmdb {
/**
 * @brief Represents a graph of operations to be performed on the database.
 */
class QueryGraph {
public:
    /**
     * @brief Constructs a new QueryGraph.
     * @param allocator The allocator to use for nodes.
     */
    explicit QueryGraph(ok::Allocator *allocator) : m_allocator{allocator} {}

    /**
     * @brief Base class for nodes in the query graph.
     */
    class Node {
    public:
        Node() = delete;

        /**
         * @brief Types of nodes in the graph.
         */
        enum class Type {
            READ,         ///< Read data from disk.
            WRITE,        ///< Write data to disk.
            ALTER_USER,   ///< Alter user properties.
            ATOMIC,       ///< A group of atomic operations.
            WRITE_COLUMN, ///< Write a value to a column.
            CALL,         ///< Call a function.
            INSERT,       ///< Insert rows into a table.
            EMIT_QUERY,   ///< Emit query results.
            DELETE_TABLE, ///< Delete a table.
        };

        /**
         * @brief Gets the type of the node.
         * @return The node type.
         */
        Type type() const {
            return m_type;
        }

        /**
         * @brief Gets the next node in the graph.
         * @return An optional pointer to the next node.
         */
        ok::Optional<Node *> next() {
            return m_next;
        }

        /**
         * @brief Sets the next node in the graph.
         * @param node Pointer to the next node.
         */
        void set_next(Node *node) {
            m_next = node;
        }

    protected:
        Node(Type type) : m_type{type} {
        }

        ok::Optional<Node *> m_next{};
        Type m_type;
    };

    /**
     * @brief Node representing a disk read operation.
     */
    class ReadNode : public Node {
    public:
        ReadNode(U64 offset, U64 size) : Node{Type::READ}, m_size{size}, m_offset{offset} {}

        U64 size() const { return m_size; }
        U64 offset() const { return m_offset; }
        ok::Slice<U8> buffer() const { return m_buffer; }
        void set_buffer(ok::Slice<U8> buffer) { m_buffer = buffer; }

    private:
        U64 m_size;
        U64 m_offset;
        ok::Slice<U8> m_buffer;
    };

    /**
     * @brief Node representing a disk write operation.
     */
    class WriteNode : public Node {
    public:
        WriteNode(U64 offset, U64 size) : Node{Type::WRITE}, m_size{size}, m_offset{offset} {}

        U64 size() const { return m_size; }
        U64 offset() const { return m_offset; }

    private:
        U64 m_size;
        U64 m_offset;
    };

    /**
     * @brief Node representing a user property alteration.
     */
    class AlterUserNode : public Node {
    public:
        enum class Property {
            PASSWORD, ///< Change the user's password.
        };

        explicit AlterUserNode(ok::StringView user_name, Property property, DBValue *property_value) : Node{Type::ALTER_USER}, m_user_name{user_name}, m_property{property}, m_property_value{property_value} {}

        ok::StringView user_name() const { return m_user_name; }
        Property property() const { return m_property; }
        DBValue *property_value() { return m_property_value; }

    private:
        ok::StringView m_user_name;
        Property m_property;
        DBValue *m_property_value;
    };

    /**
     * @brief Node representing a collection of operations that must be performed atomically.
     */
    class AtomicNode : public Node {
    public:
        explicit AtomicNode(ok::Allocator *allocator) : Node{Type::ATOMIC} {
            m_nodes = ok::List<Node *>::alloc(allocator);
        }

        void add(Node *node) { m_nodes.push(node); }

        ok::Slice<Node *> nodes() { return m_nodes.slice(); }

    private:
        ok::List<Node *> m_nodes;
    };

    /**
     * @brief Node representing an update to a table column.
     */
    class WriteColumnNode : public Node {
    public:
        WriteColumnNode(DBTable *table, UZ idx, DBValue *value) : Node{Type::WRITE_COLUMN}, m_table{table}, m_idx{idx}, m_value{value} {}

        DBTable *table() { return m_table; }
        UZ idx() { return m_idx; }
        DBValue *value() { return m_value; }

    private:
        DBTable *m_table;
        UZ m_idx;
        DBValue *m_value;
    };

    /**
     * @brief Node representing a function call.
     */
    class CallNode : public Node {
    public:
        CallNode(Allocator *allocator, StringView fn_name, Slice<DBValue *> args) : Node{Type::CALL}, m_fn_name{fn_name}, m_args{args}, m_return_value{new (allocator) DelayedDBValue{}} {}

        Slice<DBValue *> args() const { return m_args; }
        DelayedDBValue *return_value() const { return m_return_value; }
        StringView fn_name() const { return m_fn_name; }

    private:
        StringView m_fn_name;
        Slice<DBValue *> m_args;
        DelayedDBValue *m_return_value;
    };

    /**
     * @brief Node representing an insertion of rows into a table.
     */
    class InsertNode : public Node {
    public:
        using ValuesSlice = Slice<ok::Pair<StringView, DBValue *>>;

        InsertNode(DBTable *table, Slice<StringView> column_names, Slice<DBValue *> column_values, UZ rows_count) : Node{Type::INSERT}, m_table{table}, m_column_names{column_names}, m_column_values{column_values}, m_rows_count{rows_count} {}

        DBTable *table() const { return m_table; }
        Slice<StringView> column_names() const { return m_column_names; }
        Slice<DBValue *> column_values() const { return m_column_values; }
        UZ rows_count() const { return m_rows_count; }

    private:
        DBTable *m_table;
        Slice<StringView> m_column_names;
        Slice<DBValue *> m_column_values;
        UZ m_rows_count;
    };

    /**
     * @brief Node representing the emission of a query's result set.
     */
    class EmitQueryNode : public Node {
    public:
        EmitQueryNode(Allocator *allocator, Slice<DBTable *> queried_tables, Slice<StringView> column_names, Slice<DBValue *> column_values, Slice<SQL::ColumnType> column_types) : Node{Type::EMIT_QUERY}, m_queried_tables{queried_tables}, m_column_values{column_values}, m_emitted_table{new (allocator) DBTable{allocator, DBTable::F_ANON | DBTable::F_PROXY, ""_sv, column_values.count, column_names.items, column_types.items}} {}

        DBTable *table() const { return m_emitted_table; }
        Slice<DBTable *> queried_tables() const { return m_queried_tables; }
        Slice<DBValue *> column_values() const { return m_column_values; }

    private:
        Slice<DBTable *> m_queried_tables;
        Slice<DBValue *> m_column_values;
        DBTable *m_emitted_table;
    };

    /**
     * @brief Node representing a table deletion.
     */
    class DeleteTableNode : public Node {
    public:
        explicit DeleteTableNode(DBTable *table) : Node{Type::DELETE_TABLE}, m_table{table} {}

        DBTable *table() const { return m_table; }

    private:
        DBTable *m_table;
    };

    /**
     * @brief Gets the root node of the graph.
     * @return An optional pointer to the root node.
     */
    ok::Optional<Node *> root_node() const { return m_root_node; }

    /**
     * @brief Adds a read node to the graph.
     * @return The added node.
     */
    ReadNode *read(U64 offset, U64 size) { return add_generic_node<ReadNode>(offset, size); }

    /**
     * @brief Adds a write node to the graph.
     * @return The added node.
     */
    WriteNode *write(U64 offset, U64 size) { return add_generic_node<WriteNode>(offset, size); }

    /**
     * @brief Adds an atomic node to the graph.
     * @return The added node.
     */
    AtomicNode *atomic() { return add_generic_node<AtomicNode>(m_allocator); }

    /**
     * @brief Adds a write column node to the graph.
     * @return The added node.
     */
    WriteColumnNode *write_column(DBTable *table, UZ idx, DBValue *value) { return add_generic_node<WriteColumnNode>(table, idx, value); }

    /**
     * @brief Adds a function call node to the graph.
     * @return The added node.
     */
    CallNode *call(ok::Allocator *allocator, ok::StringView fn_name, Slice<DBValue *> args) { return add_generic_node<CallNode>(allocator, fn_name, args); }

    /**
     * @brief Adds an emit query node to the graph.
     * @return The added node.
     */
    EmitQueryNode *emit_query(Allocator *allocator, Slice<DBTable *> queried_tables, Slice<StringView> column_names, Slice<DBValue *> column_values, Slice<SQL::ColumnType> column_types) { return add_generic_node<EmitQueryNode>(allocator, queried_tables, column_names, column_values, column_types); }

    /**
     * @brief Adds a delete table node to the graph.
     * @return The added node.
     */
    DeleteTableNode *delete_table(DBTable *table) { return add_generic_node<DeleteTableNode>(table); }

    /**
     * @brief Adds an insert node to the graph.
     * @return The added node.
     */
    InsertNode *insert(DBTable *table, Slice<StringView> names, Slice<DBValue *> values, UZ rows_count) { return add_generic_node<InsertNode>(table, names, values, rows_count); }

    /**
     * @brief Resets the query graph, clearing all nodes.
     * @return The added node.
     */
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

    ok::Allocator *m_allocator;
    ok::Optional<Node *> m_root_node;
    ok::Optional<Node *> m_last_node;
};
} // namespace xmdb
