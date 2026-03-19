#pragma once

#include <csetjmp>

#include "util.hpp"
#include "DBDescriptor.hpp"
#include "DBTable.hpp"
#include "DBValue.hpp"
#include "DBRecord.hpp"
#include "QueryGraph.hpp"
#include "StaticStorage.hpp"

#define XMDB_FAIL(ctx, msg)                                             \
    do {                                                                \
        /* TODO(oleh): Retrieve location from the query executed. */    \
        (ctx)->error =                                                  \
            ErrorWithSourceLocation{.message = String::alloc((ctx)->allocator, msg), .location = {}}; \
        longjmp((ctx)->jmpbuf, 1);                                      \
                                                                        \
    } while (false)

#define XMDB_FAIL_FMT(ctx, fmt, ...)                                    \
    do {                                                                \
        /* TODO(oleh): Retrieve location from the query executed. */    \
        (ctx)->error =                                                  \
            ErrorWithSourceLocation{.message = String::format((ctx)->allocator, fmt, __VA_ARGS__), .location = {}}; \
        longjmp((ctx)->jmpbuf, 1);                                      \
    } while (false)

namespace xmdb {
/**
 * @brief Context for executing a database query, managing variables, tables, and operations.
 * Note that the execution context does not actually perform any work (despite it's name) besides setting up all
 * data that is required for a query to be executed. It only appends new nodes to the query_graph field.
 */
struct QueryExecutionContext {
    /**
     * @brief Fetches a variable value by index.
     * @param index The variable index.
     * @return Pointer to the DBValue.
     */
    DBValue *fetch_var(U32 index);

    /**
     * @brief Puts a value into a variable.
     * @param index The variable index.
     * @param name The variable name.
     * @param value Pointer to the DBValue.
     */
    void put_var(U32 index, StringView name, DBValue *value);

    /**
     * @brief Fetches a value from a table column.
     * @param table The table to fetch from.
     * @param column_name The name of the column.
     * @return Pointer to the DBValue.
     */
    DBValue *fetch_column(DBTable *table, StringView column_name);

    /**
     * @brief Emits a column value for a query result.
     * @param table The table being emitted from.
     * @param value Pointer to the value.
     * @param column_name The name of the column in the result.
     */
    void emit_column(DBTable *table, DBValue *value, StringView column_name);

    /**
     * @brief Fetches a table by name.
     * @param name The table name.
     * @return Pointer to the DBTable.
     */
    DBTable *fetch_table(StringView name);

    /**
     * @brief Fetches a table by index.
     * @param index The table index.
     * @return Pointer to the DBTable.
     */
    DBTable *fetch_table(U32 index);

    /**
     * @brief Stores a table by index.
     * @param index The table index.
     * @param table Pointer to the DBTable.
     */
    void put_table(U32 index, DBTable *table);

    /**
     * @brief Creates a new table based on a schema.
     * @param name The table name.
     * @param schema Pointer to the table schema.
     */
    void create_table(StringView name, SQL::TableSchema *schema);

    /**
     * @brief Emits the results of a query as a new table.
     * @param index The query index.
     * @param column_types Pointer to the types of the emitted columns.
     * @return Pointer to the resulting DBTable.
     */
    DBTable *emit_query(U32 index, SQL::ColumnType *column_types);

    /**
     * @brief Prepares a column value for insertion.
     * @param column_name The name of the column.
     * @param value Pointer to the value.
     */
    void insert_column(StringView column_name, DBValue *value);

    /**
     * @brief Signals that a row is ready to be inserted.
     */
    void insert_row();

    /**
     * @brief Commits pending row insertions to a table.
     * @param table The table to insert into.
     */
    void commit_insert(DBTable *table);

    /**
     * @brief Prepares a column update.
     * @param table The table to update.
     * @param column_name The column to update.
     * @param value Pointer to the new value.
     */
    void update_column(DBTable *table, StringView column_name, DBValue *value);

    /**
     * @brief Commits pending updates.
     */
    void commit_update();

    /**
     * @brief Deletes a table.
     * @param table The table to delete.
     */
    void delete_table(DBTable *table);

    /**
     * @brief Creates a new database user.
     * @param name The user name.
     */
    void create_user(StringView name);

    /**
     * @brief Prepares an alteration to a user property.
     * @param user_name The name of the user.
     * @param property_name The property to alter.
     * @param value Pointer to the new property value.
     */
    void alter_user_property(StringView user_name, StringView property_name, DBValue *value);

    /**
     * @brief Commits pending user alterations.
     */
    void commit_alter_user();

    /**
     * @brief Compares two values.
     * @param lhs Left-hand side value.
     * @param rhs Right-hand side value.
     * @return Pointer to a DBValue representing the comparison result.
     */
    DBValue *compare(DBValue *lhs, DBValue *rhs);

    /**
     * @brief Prepares an argument for a function call.
     * @param arg Pointer to the argument value.
     */
    void prepare_call_arg(DBValue *arg);

    /**
     * @brief Calls a built-in function.
     * @param fn_name The function name.
     * @param args_count The number of arguments.
     * @return Pointer to the result DBValue.
     */
    DBValue *call(StringView fn_name, U64 args_count);

    QueryExecutionContext *next;             ///< Pointer to the next context in a pool.
    StaticStorage *static_storage;           ///< Pointer to global static storage.
    QueryGraph query_graph;                  ///< The graph of operations to perform.
    ok::Allocator *allocator;                ///< The allocator for this execution.
    DBUser *user;                            ///< The user performing the query.
    ok::Table<U32, DBValue *> vars;          ///< Local variables.
    ok::Table<U32, DBTable *> tables;        ///< Tables referenced in the query.
    List<StringView> insert_column_names;    ///< Pending column names for insertion.
    List<DBValue *> insert_column_values;    ///< Pending column values for insertion.
    List<ok::Pair<Slice<StringView>, Slice<DBValue *>>> rows_to_insert; ///< Rows pending insertion.
    ok::MultiList<StringView, DBValue *, DBTable *> emitted_columns; ///< Columns emitted for the result.
    ok::MultiList<StringView, DBValue *> columns_to_update; ///< Columns pending update.
    Optional<QueryGraph::AtomicNode *> alter_user_atomic_node; ///< Atomic node for user alterations.
    DBDescriptor *current_db;                ///< The database being queried.
    Optional<DBTable *> table_to_insert;     ///< Target table for insertion.
    Optional<DBTable *> table_to_update;     ///< Target table for update.
    Optional<DBTable *> last_emitted_query;  ///< The last table emitted by a query.
    List<DBValue *> call_args;               ///< Arguments for the next function call.
    Optional<ErrorWithSourceLocation> error; ///< Error information if execution failed.
    jmp_buf jmpbuf;                          ///< Jump buffer for error handling.
};
} // namespace xmdb
