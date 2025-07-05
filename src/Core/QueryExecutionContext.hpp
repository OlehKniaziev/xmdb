#pragma once

#include "DBDescriptor.hpp"
#include "DBTable.hpp"
#include "DBValue.hpp"

namespace xmdb {
struct QueryExecutionContext {
    DBValue fetch_var(U32);
    void put_var(U32, StringView, DBValue);

    DBValue fetch_column(DBTable *, StringView);
    void emit_column(DBValue, StringView);

    DBTable *fetch_table(StringView);
    void create_table(StringView, SQL::TableSchema *);

    DBTable *emit_query(U32, SQL::ColumnType *);

    ok::Allocator *allocator;
    QueryExecutionContext *next;
    ok::Table<U32, DBValue> vars;
    ok::MultiList<StringView, DBValue> emitted_columns;
    DBDescriptor *current_db;
    Optional<DBTable *> last_emitted_query;
};
} // namespace xmdb
