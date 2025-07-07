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

    void insert_column(DBTable *, StringView, DBValue);
    void insert_row(DBTable *);

    void commit_insert();

    ok::Allocator *allocator;
    QueryExecutionContext *next;
    ok::Table<U32, DBValue> vars;
    ok::MultiList<StringView, DBValue> emitted_columns;
    UZ emitted_columns_offset;
    ok::MultiList<StringView, DBValue> columns_to_insert;
    ok::Table<DBTable *, ok::MultiList<UZ, StringView *, DBValue *>> rows_to_insert;
    DBDescriptor *current_db;
    Optional<DBTable *> last_emitted_query;
};
} // namespace xmdb
