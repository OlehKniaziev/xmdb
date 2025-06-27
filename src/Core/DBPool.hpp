#pragma once

#include "DBValue.hpp"
#include "DBTable.hpp"
#include "ok.hpp"

#include "QueryExecutionContext.hpp"

namespace xmdb {
struct DBPool {
    DBPool(ok::Allocator *);

    QueryExecutionContext *rent_empty_execution_context();
    void return_execution_context(QueryExecutionContext *);

    ok::Allocator *allocator;
    QueryExecutionContext *execution_contexts;
};
} // namespace xmdb
