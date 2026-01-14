#include "DBPool.hpp"

using namespace ok::literals;

namespace xmdb {
DBPool::DBPool(ok::Allocator *allocator) : allocator{allocator}, execution_contexts{nullptr} {
    const StringView default_db_name = "default"_sv;
    DBDescriptor *db = create_db(default_db_name);
    DBUser *admin = allocator->alloc<DBUser>();
    *admin = DBUser::admin();
    db->add_user(admin);
}

DBDescriptor *DBPool::get_db(StringView db_name) {
    for (DBDescriptor *curr_db = db_descriptors;
         curr_db != nullptr;
         curr_db = curr_db->next) {
        if (curr_db->name == db_name) return curr_db;
    }

    OK_PANIC_FMT("No database descriptor with name '" OK_SV_FMT "' found", OK_SV_ARG(db_name));
}

DBDescriptor *DBPool::create_db(StringView db_name) {
    DBDescriptor *db = DBDescriptor::alloc(allocator, db_name);
    db->next = db_descriptors;
    db_descriptors = db;
    return db;
}

QueryExecutionContext *DBPool::rent_empty_execution_context(DBDescriptor *db, DBUser *user) {
    if (execution_contexts) {
        QueryExecutionContext *ctx = execution_contexts;
        ctx->query_graph.reset();
        ctx->current_db = db;
        ctx->user = user;
        ctx->vars.clear();
        ctx->tables.clear();
        ctx->emitted_columns.count = 0;
        ctx->columns_to_insert.count = 0;
        ctx->rows_to_insert.count = 0;
        ctx->columns_to_update.count = 0;
        ctx->table_to_update = {};
        ctx->last_emitted_query = {};
        ctx->alter_user_atomic_node = {};
        execution_contexts = execution_contexts->next;
        return ctx;
    }

    QueryExecutionContext *ctx = allocator->alloc<QueryExecutionContext>();
    ctx->next = nullptr;
    ctx->query_graph = QueryGraph{allocator};
    ctx->allocator = allocator;
    ctx->user = user;
    ctx->vars = ok::Table<U32, DBValue *>::alloc(allocator);
    ctx->tables = ok::Table<U32, DBTable *>::alloc(allocator);
    ctx->emitted_columns = ok::MultiList<StringView, DBValue *, DBTable *>::alloc(allocator);
    ctx->columns_to_insert = ok::MultiList<StringView, DBValue *>::alloc(allocator);
    ctx->rows_to_insert = ok::MultiList<UZ, StringView *, DBValue **>::alloc(allocator);
    ctx->columns_to_update = ok::MultiList<StringView, DBValue *>::alloc(allocator);
    ctx->current_db = db;
    ctx->table_to_update = {};
    ctx->last_emitted_query = {};
    ctx->alter_user_atomic_node = {};
    return ctx;
}

void DBPool::return_execution_context(QueryExecutionContext *ctx) {
    ctx->next = execution_contexts;
    execution_contexts = ctx;
}
} // namespace xmdb
