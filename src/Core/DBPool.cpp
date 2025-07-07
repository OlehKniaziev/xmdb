#include "DBPool.hpp"

using namespace ok::literals;

namespace xmdb {
DBPool::DBPool(ok::Allocator *allocator) : allocator{allocator}, execution_contexts{nullptr} {
    DBDescriptor *default_db = DBDescriptor::alloc(allocator, "default"_sv);
    db_descriptors = default_db;
}

DBDescriptor *DBPool::get_db(StringView db_name) {
    for (DBDescriptor *curr_db = db_descriptors;
         curr_db != nullptr;
         curr_db = curr_db->next) {
        if (curr_db->name == db_name) return curr_db;
    }

    OK_PANIC_FMT("No database descriptor with name '" OK_SV_FMT "' found", OK_SV_ARG(db_name));
}

QueryExecutionContext *DBPool::rent_empty_execution_context(DBDescriptor *db) {
    if (execution_contexts) {
        QueryExecutionContext *ctx = execution_contexts;
        execution_contexts = execution_contexts->next;
        return ctx;
    }

    QueryExecutionContext *ctx = allocator->alloc<QueryExecutionContext>();
    ctx->allocator = allocator;
    ctx->next = nullptr;
    ctx->vars = ok::Table<U32, DBValue>::alloc(allocator);
    ctx->emitted_columns = ok::MultiList<StringView, DBValue>::alloc(allocator);
    ctx->emitted_columns_offset = 0;
    ctx->columns_to_insert = ok::MultiList<StringView, DBValue>::alloc(allocator);
    ctx->rows_to_insert = ok::Table<DBTable *, ok::MultiList<UZ, StringView *, DBValue *>>::alloc(allocator);
    ctx->current_db = db;
    ctx->last_emitted_query = {};
    return ctx;
}

void DBPool::return_execution_context(QueryExecutionContext *ctx) {
    ctx->next = execution_contexts;
    execution_contexts = ctx;
}
} // namespace xmdb
