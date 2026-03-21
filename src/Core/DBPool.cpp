#include "DBPool.hpp"
#include "Logger.hpp"
#include "catalog.hpp"

using namespace ok::literals;

namespace xmdb {
constexpr ok::StringView DEFAULT_CATALOG_FILE_NAME = "xmdb.cat"_sv;

DBPool::DBPool(ok::Allocator *allocator, Flags flags) : allocator{allocator},
                                                        flags{flags},
                                                        execution_contexts{nullptr} {
    XMDB_FIXME("Replace this constructor by either a static method or a function, it can fail");

#ifdef XMDB_TEST
    this->flags |= F_EPHEMERAL;
#endif // XMDB_TEST

    catalog_file_name = DEFAULT_CATALOG_FILE_NAME;

    if (!ephemeral() && ok::File::exists(catalog_file_name)) {
        log::info("Found a catalog file in the current directory (" OK_SV_FMT "), loading it",
                  OK_SV_ARG(catalog_file_name));

        Result<UZ, ok::String> cat_load_result = catalog_load(allocator, this, catalog_file_name);
        if (!cat_load_result.ok()) {
            OK_PANIC_FMT("Failed to load the catalog: %s",
                         cat_load_result.error().cstr());
        }
    } else {
        const StringView default_db_name = "default"_sv;
        DBDescriptor *db = DBDescriptor::alloc(allocator, default_db_name);
        db->next = nullptr;
        this->db_descriptors = db;

        DBUser *admin = allocator->alloc<DBUser>();
        *admin = DBUser::admin();
        db->add_user(admin);

        if (!ephemeral()) {
            Result<UZ, ok::String> cat_save_result = catalog_save(allocator, this, catalog_file_name);
            if (!cat_save_result.ok()) {
                OK_PANIC_FMT("Failed to save a catalog to disk: %s", cat_save_result.error().cstr());
            }
        }
    }
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
        ctx->static_storage = make_or_get_static_storage();
        ctx->query_graph.reset();
        ctx->db_pool = this;
        ctx->current_db = db;
        ctx->user = user;
        ctx->vars.clear();
        ctx->tables.clear();
        ctx->emitted_columns.count = 0;
        ctx->insert_column_names.count = 0;
        ctx->insert_column_values.count = 0;
        ctx->rows_to_insert.count = 0;
        ctx->columns_to_update.count = 0;
        ctx->call_args.count = 0;
        ctx->table_to_update = {};
        ctx->last_emitted_query = {};
        ctx->alter_user_atomic_node = {};
        execution_contexts = execution_contexts->next;
        return ctx;
    }

    QueryExecutionContext *ctx = allocator->alloc<QueryExecutionContext>();
    ctx->next = nullptr;
    ctx->static_storage = make_or_get_static_storage();
    ctx->query_graph = QueryGraph{allocator};
    ctx->allocator = allocator;
    ctx->user = user;
    ctx->vars = ok::Table<U32, DBValue *>::alloc(allocator);
    ctx->tables = ok::Table<U32, DBTable *>::alloc(allocator);
    ctx->emitted_columns = ok::MultiList<StringView, DBValue *, DBTable *>::alloc(allocator);
    ctx->insert_column_names = ok::List<StringView>::alloc(allocator);
    ctx->insert_column_values = ok::List<DBValue *>::alloc(allocator);
    ctx->rows_to_insert = ok::List<ok::Pair<Slice<StringView>, Slice<DBValue *>>>::alloc(allocator);
    ctx->columns_to_update = ok::MultiList<StringView, DBValue *>::alloc(allocator);
    ctx->call_args = ok::List<DBValue *>::alloc(allocator);
    ctx->db_pool = this;
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

Result<UZ, ok::String> sync_db_pool(DBPool *pool) {
    if (!pool->ephemeral()) {
        return catalog_save(pool->allocator, pool, pool->catalog_file_name);
    } else {
        return 0;
    }
}
} // namespace xmdb
