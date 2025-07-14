#pragma once

#include "DBValue.hpp"
#include "DBTable.hpp"
#include "ok.hpp"

#include "QueryExecutionContext.hpp"

namespace xmdb {
struct DBPool {
    DBPool(ok::Allocator *);

    DBDescriptor *get_db(StringView);
    DBDescriptor *create_db(StringView);

    QueryExecutionContext *rent_empty_execution_context(DBDescriptor *);
    void return_execution_context(QueryExecutionContext *);

    ok::Allocator *allocator;
    QueryExecutionContext *execution_contexts;
    DBDescriptor *db_descriptors;
};
} // namespace xmdb
