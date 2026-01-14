#pragma once

#include <csetjmp>

#include "util.hpp"
#include "DBDescriptor.hpp"
#include "DBTable.hpp"
#include "DBValue.hpp"
#include "QueryGraph.hpp"

#define XMDB_FAIL(ctx, msg)                                                                                        \
    do {                                                                                                               \
        /* TODO(oleh): Retrieve location from the query executed. */                                                   \
(ctx)->error =                                                                                                   \
    ErrorWithSourceLocation{.message = String::alloc((ctx)->allocator, msg), .location = {}};  \
longjmp((ctx)->jmpbuf, 1);                                                                                       \
                                                                                                                       \
    } while (false)

#define XMDB_FAIL_FMT(ctx, fmt, ...)                                                                                        \
    do {                                                                                                               \
        /* TODO(oleh): Retrieve location from the query executed. */                                                   \
(ctx)->error =                                                                                                   \
    ErrorWithSourceLocation{.message = String::format((ctx)->allocator, fmt, __VA_ARGS__), .location = {}};  \
longjmp((ctx)->jmpbuf, 1);                                                                                       \
                                                                                                                       \
    } while (false)



namespace xmdb {
struct QueryExecutionContext {
    DBValue * fetch_var(U32);
    void put_var(U32, StringView, DBValue *);

    DBValue * fetch_column(DBTable *, StringView);
    void emit_column(DBTable *, DBValue *, StringView);

    DBTable *fetch_table(StringView);
    DBTable *fetch_table(U32);

    void put_table(U32, DBTable *);

    void create_table(StringView, SQL::TableSchema *);

    DBTable *emit_query(U32, SQL::ColumnType *);

    void insert_column(DBTable *, StringView, DBValue *);
    void insert_row();

    void commit_insert();

    void update_column(DBTable *, StringView, DBValue *);
    void commit_update();

    void delete_table(DBTable *);

    void create_user(StringView);
    void alter_user_property(StringView, StringView, DBValue *);
    void commit_alter_user();

    DBValue * compare(DBValue *, DBValue *);

    QueryExecutionContext *next;
    QueryGraph query_graph;
    ok::Allocator *allocator;
    DBUser *user;
    ok::Table<U32, DBValue *> vars;
    ok::Table<U32, DBTable *> tables;
    ok::MultiList<StringView, DBValue *, DBTable *> emitted_columns;
    ok::MultiList<StringView, DBValue *> columns_to_insert;
    ok::MultiList<UZ, StringView *, DBValue **> rows_to_insert;
    ok::MultiList<StringView, DBValue *> columns_to_update;
    Optional<QueryGraph::AtomicNode *> alter_user_atomic_node;
    DBDescriptor *current_db;
    Optional<DBTable *> table_to_insert;
    Optional<DBTable *> table_to_update;
    Optional<DBTable *> last_emitted_query;
    Optional<ErrorWithSourceLocation> error;
    // TODO(oleh): Abstract this probably so it's easier to port later on.
    jmp_buf jmpbuf;
};
} // namespace xmdb
