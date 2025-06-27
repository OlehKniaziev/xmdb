#pragma once

#include "DBDescriptor.hpp"
#include "DBTable.hpp"
#include "DBValue.hpp"

namespace xmdb {
struct QueryExecutionContext {
    DBValue fetch_var(U32);
    void put_var(U32, StringView, DBValue);

    DBValue fetch_column(DBTable *, StringView);

    DBTable *fetch_table(StringView);

    QueryExecutionContext *next;
    ok::Table<U32, DBValue> vars;
    DBDescriptor *current_db;
};
} // namespace xmdb
