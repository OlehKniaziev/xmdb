#include "QueryExecutionContext.hpp"
#include "new.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "Png.hpp"

using namespace ok::literals;

namespace xmdb {
#define X(p, _unused)                                                                                                  \
    void CHECK_##p(QueryExecutionContext *ctx) {                                                                       \
        if ((ctx->user->perm & PERM_##p) == 0) {                                                                       \
            XMDB_FAIL_FMT(ctx, "user " OK_SV_FMT " does not have " #p " permissions", OK_SV_ARG(ctx->user->name));          \
        }                                                                                                              \
    }

XMDB_ENUM_USER_PERMISSIONS

#undef X

Result<UZ, ok::String> sync_db_pool(DBPool *);

void QueryExecutionContext::sync_state() {
    auto result = sync_db_pool(db_pool);
    if (!result.ok()) {
        XMDB_FAIL_FMT(this,
                      "Failed to write database state to disk: %s",
                      result.error().cstr());
    }
}

DBValue *QueryExecutionContext::fetch_var(U32 ip) {
    CHECK_READ(this);
    Optional<DBValue *> value = vars.get(ip);
    return value.get();
}

void QueryExecutionContext::put_var(U32 ip, StringView name, DBValue *value) {
    OK_UNUSED(name);
    CHECK_WRITE(this);
    vars.put(ip, value);
}

DBValue *QueryExecutionContext::fetch_column(DBTable *table, StringView column_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < table->columns_count(); ++i) {
        if (table->columns_names()[i] == column_name) {
            ColumnLayout layout = table->layout().columns[i];
            SQL::ColumnType column_type = table->columns_types()[i];
            SQL::Type value_type = column_type_to_type(column_type);
            return new (allocator) ColumnDBValue{value_type, layout, table};
        }
    }

    OK_UNREACHABLE();
}

DBTable *QueryExecutionContext::fetch_table(StringView table_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < current_db->tables.count; ++i) {
        DBTable *table = current_db->tables[i];
        if (table->name() == table_name) {
            return table;
        }
    }

    OK_UNREACHABLE();
}

void QueryExecutionContext::create_table(StringView table_name, SQL::TableSchema *table_schema) {
    CHECK_WRITE(this);

    if (!table_schema->columns_types.has_value()) {
        OK_TODO();
    }

    XMDB_FIXME("Add 'CREATE TABLE' node to the query graph");

    UZ columns_count = table_schema->columns_names.count;
    List<SQL::ColumnType> column_types = table_schema->columns_types.value;
    ok::StringView *column_names_ptr = allocator->alloc<ok::StringView>(columns_count);
    ok::Slice<ok::StringView> column_names = {column_names_ptr, columns_count};
    for (UZ i = 0; i < table_schema->columns_names.count; ++i) {
        if (table_schema->columns_names[i].has_value()) {
            column_names[i] = table_schema->columns_names[i].value.view();
        } else {
            column_names[i] = ""_sv;
        }
    }

    DBTable::Flags table_flags = DBTable::F_PERSIST;

#ifdef XMDB_TEST
    table_flags |= DBTable::F_EPHEMERAL;
#endif // XMDB_TEST

    DBTable *new_table = current_db->create_new_table(allocator,
                                                      table_name,
                                                      table_flags,
                                                      column_types.count,
                                                      column_names,
                                                      column_types.slice());

    if (new_table == nullptr) {
        XMDB_FAIL_FMT(this,
                      "Failed to create table '" OK_SV_FMT "', see the log for details",
                      OK_SV_ARG(table_name));
    }

    sync_state();
}

void QueryExecutionContext::emit_column(DBTable *table, DBValue *column_value, StringView column_name) {
    emitted_columns.push(column_name, column_value, table);
}

DBTable *QueryExecutionContext::emit_query(U32 column_count, SQL::ColumnType *column_types) {
    const StringView *column_names_ptr = emitted_columns.get_items<StringView>();
    DBValue **const column_values_ptr = emitted_columns.get_items<DBValue *>();
    DBTable **tables = emitted_columns.get_items<DBTable *>();

    StringView *column_names = allocator->alloc<StringView>(column_count);
    DBValue **column_values = allocator->alloc<DBValue *>(column_count);
    memcpy(column_names, column_names_ptr, column_count * sizeof(*column_names));
    memcpy(column_values, column_values_ptr, column_count * sizeof(*column_values));

    emitted_columns.count = 0;

    Slice<DBTable *> queried_tables_slice = {tables, column_count};
    Slice<StringView> column_names_slice = {column_names, column_count};
    Slice<DBValue *> column_values_slice = {column_values, column_count};
    Slice<SQL::ColumnType> column_types_slice = {column_types, column_count};

    QueryGraph::EmitQueryNode *query_node = query_graph.emit_query(allocator,
                                                                   queried_tables_slice,
                                                                   column_names_slice,
                                                                   column_values_slice,
                                                                   column_types_slice);
    DBTable *table = query_node->table();
    last_emitted_query = table;
    return table;
}

void QueryExecutionContext::insert_column(StringView column_name, DBValue *column_value) {
    CHECK_WRITE(this);

    insert_column_names.push(column_name);
    insert_column_values.push(column_value);
}

void QueryExecutionContext::insert_row() {
    Slice<StringView> column_names = insert_column_names.copy(allocator).slice();
    Slice<DBValue *> column_values = insert_column_values.copy(allocator).slice();
    insert_column_names.count = 0;
    insert_column_values.count = 0;
    rows_to_insert.push({column_names, column_values});
}

void QueryExecutionContext::commit_insert(DBTable *table) {
    CHECK_WRITE(this);

    Slice<StringView> insert_column_names{};
    List<DBValue *> insert_column_values = List<DBValue *>::alloc(allocator);

    for (UZ i = 0; i < rows_to_insert.count; ++i) {
        ok::Pair<Slice<StringView>, Slice<DBValue *>> row = rows_to_insert[i];
        insert_column_names = row.a;

        for (UZ val_idx = 0; val_idx < row.b.count; ++val_idx) {
            DBValue *val = row.b[val_idx];
            insert_column_values.push(val);
        }
    }

    query_graph.insert(table,
                       insert_column_names.slice(),
                       insert_column_values.slice(),
                       rows_to_insert.count);

    rows_to_insert.count = 0;
}

void QueryExecutionContext::update_column(DBTable *table, StringView column_name, DBValue *column_value) {
    if (!table_to_update.has_value()) {
        table_to_update = table;
    } else if (table_to_update.value != table) {
        OK_PANIC("Table to update does not match the given table");
    }

    columns_to_update.push(column_name, column_value);
}

void QueryExecutionContext::commit_update() {
    CHECK_WRITE(this);

    DBTable *table = table_to_update.get();
    StringView *columns_to_update_names = columns_to_update.get_items<StringView>();
    DBValue **columns_to_update_values = columns_to_update.get_items<DBValue *>();

    if (table->flags() & DBTable::F_PERSIST) {
        Slice<StringView> column_names = table->columns_names();

        for (UZ uc = 0; uc < columns_to_update.count; ++uc) {
            StringView column_name = columns_to_update_names[uc];
            UZ idx = column_names.find_index(column_name);
            OK_ASSERT(idx != (U64) -1);

            query_graph.write_column(table, idx, columns_to_update_values[uc]);
        }
    } else {
        Slice<DBValue *> values = table->proxy_column_values();

        for (UZ i = 0; i < columns_to_update.count; ++i) {
            StringView column_name = columns_to_update_names[i];
            for (UZ c = 0; c < table->columns_count(); ++c) {
                if (table->columns_names()[c] != column_name) continue;
                DBValue *new_value = columns_to_update_values[i];
                values[c] = new_value;
                break;
            }
        }
    }

    columns_to_update.count = 0;
    table_to_update = {};
}

void QueryExecutionContext::delete_table(DBTable *table) {
    CHECK_WRITE(this);

    query_graph.delete_table(table);
}

void QueryExecutionContext::create_user(StringView name) {
    DBUser *user = allocator->alloc<DBUser>();
    *user = {name, ""_sv, PERM_READ | PERM_WRITE};
    current_db->add_user(user);

    sync_state();
}

void QueryExecutionContext::alter_user_property(StringView user_name, StringView property_name, DBValue *value) {
    CHECK_ADMIN(this);

    if (!alter_user_atomic_node) {
        alter_user_atomic_node = query_graph.atomic();
    }

    QueryGraph::AtomicNode *atomic_node = alter_user_atomic_node.get();

    if (property_name == "PASSWORD"_sv) {
        if (value->type() != SQL::TYPE_STRING)
            XMDB_FAIL_FMT(this, "expected user password to be of type string, got type '%s' instead",
                     SQL::type_name(value->type()));

        auto *alter_node = new (allocator) QueryGraph::AlterUserNode{user_name, QueryGraph::AlterUserNode::Property::PASSWORD, value};
        atomic_node->add(alter_node);
    } else {
        XMDB_FAIL_FMT(this, "user '" OK_SV_FMT "' does not have property '" OK_SV_FMT "'", OK_SV_ARG(user_name),
                 OK_SV_ARG(property_name));
    }
}

void QueryExecutionContext::commit_alter_user() {
    alter_user_atomic_node = {};
}

DBValue *QueryExecutionContext::compare(DBValue *lhs, DBValue *rhs) {
    return new (allocator) CompareDBValue{lhs, rhs};
}

DBTable *QueryExecutionContext::fetch_table(U32 ip) {
    return tables.get(ip).get();
}

void QueryExecutionContext::put_table(U32 ip, DBTable *table) {
    tables.put(ip, table);
}

void QueryExecutionContext::prepare_call_arg(DBValue *arg_value) {
    call_args.push(arg_value);
}

DBValue *QueryExecutionContext::call(ok::StringView fn_name, U64 arg_count) {
    OK_ASSERT(call_args.count == arg_count);

    Slice<DBValue *> args = call_args.slice();
    call_args.count = 0;

    QueryGraph::CallNode *call_node = query_graph.call(allocator, fn_name, args);
    return call_node->return_value();
}
} // namespace xmdb
