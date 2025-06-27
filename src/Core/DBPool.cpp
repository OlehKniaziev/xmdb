#include "DBPool.hpp"

namespace xmdb {
DBPool::DBPool(ok::Allocator *allocator) : allocator{allocator} {
}

QueryExecutionContext *DBPool::rent_empty_execution_context() {
    if (execution_contexts) {
        QueryExecutionContext *ctx = execution_contexts;
        execution_contexts = execution_contexts->next;
        return ctx;
    }

    QueryExecutionContext *ctx = allocator->alloc<QueryExecutionContext>();
    return ctx;
}

void DBPool::return_execution_context(QueryExecutionContext *ctx) {
    ctx->next = execution_contexts;
    execution_contexts = ctx;
}
} // namespace xmdb
