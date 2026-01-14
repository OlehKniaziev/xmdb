#include "QueryExecutionContext.hpp"
#include "new.hpp"
#include "Logger.hpp"

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
            ColumnLayout layout = table->columns_layout()[i];
            SQL::ColumnType column_type = table->columns_types()[i];
            SQL::Type value_type = column_type_to_type(column_type);
            return new (allocator) ColumnDBValue{value_type, layout};
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

    List<SQL::ColumnType> column_types = table_schema->columns_types.value;
    ok::StringView *column_names = allocator->alloc<ok::StringView>(table_schema->columns_names.count);
    for (UZ i = 0; i < table_schema->columns_names.count; ++i) {
        if (table_schema->columns_names[i].has_value()) {
            column_names[i] = table_schema->columns_names[i].value.view();
        } else {
            column_names[i] = ""_sv;
        }
    }

    DBTable *new_table = new /* (this->allocator) */ DBTable(allocator, DBTable::F_PERSIST, table_name, column_types.count, column_names, column_types.items);
    current_db->tables.push(new_table);
}

void QueryExecutionContext::emit_column(DBTable *table, DBValue *column_value, StringView column_name) {
    emitted_columns.push(column_name, column_value, table);
}

DBTable *QueryExecutionContext::emit_query(U32 columns_count, SQL::ColumnType *columns_types) {
    XMDB_FIXME("This *should* also be added to the query graph, since we want every operation to preserve it's order as in the source code");

    const StringView *columns_names_ptr = emitted_columns.get_items<StringView>();
    DBValue ** const columns_values_ptr = emitted_columns.get_items<DBValue *>();
    DBTable **tables = emitted_columns.get_items<DBTable *>();

    StringView *columns_names = allocator->alloc<StringView>(columns_count);
    DBValue **columns_values = allocator->alloc<DBValue *>(columns_count);
    memcpy(columns_names, columns_names_ptr, columns_count * sizeof(*columns_names));
    memcpy(columns_values, columns_values_ptr, columns_count * sizeof(*columns_values));

    // NOTE(oleh): This is gonna be wrong once the row filtering is added.
    UZ rows_count = 0;

    for (UZ i = 0; i < emitted_columns.count; ++i) {
        DBTable *table = tables[i];
        if (table->rows_count() > rows_count) {
            rows_count = table->rows_count();
        }
    }

    emitted_columns.count = 0;

    DBTable *table = new (allocator) DBTable{allocator, DBTable::F_ANON | DBTable::F_PROXY, ""_sv, columns_count, columns_names, columns_types};
    last_emitted_query = table;
    return table;
}

void QueryExecutionContext::insert_column(DBTable *table, StringView column_name, DBValue *column_value) {
    if (!table_to_insert) {
        table_to_insert = table;
    } else {
        OK_ASSERT(table == table_to_insert.get());
    }

    columns_to_insert.push(column_name, column_value);
}

void QueryExecutionContext::insert_row() {
    OK_ASSERT(table_to_insert);

    StringView *columns_names_ptr = columns_to_insert.get_items<StringView>();
    DBValue **columns_values_ptr = columns_to_insert.get_items<DBValue *>();

    StringView *columns_names = allocator->alloc<StringView>(columns_to_insert.count);
    DBValue **columns_values = allocator->alloc<DBValue *>(columns_to_insert.count);
    memcpy(columns_names, columns_names_ptr, sizeof(*columns_names) * columns_to_insert.count);
    memcpy(columns_values, columns_values_ptr, sizeof(*columns_values) * columns_to_insert.count);

    rows_to_insert.push(columns_to_insert.count, columns_names, columns_values);

    columns_to_insert.count = 0;
}

void QueryExecutionContext::commit_insert() {
    CHECK_WRITE(this);

    OK_ASSERT(table_to_insert);

    DBTable *table = table_to_insert.get();

    if (table->flags() & DBTable::F_PERSIST) {
        XMDB_FAIL(this, "Insertion into a table on-disk is not implemented yet");
    }

    Slice<DBValue *> table_columns_values = table->proxy_column_values();

    auto &rows = rows_to_insert;

    UZ *rows_to_insert_columns_count = rows.get_items<UZ>();
    StringView **rows_to_insert_columns_names = rows.get_items<StringView *>();
    DBValue ***rows_to_insert_columns_values = rows.get_items<DBValue **>();

    for (UZ row_idx = 0; row_idx < rows.count; ++row_idx) {
        UZ columns_to_insert_count = rows_to_insert_columns_count[row_idx];
        StringView *columns_to_insert_names = rows_to_insert_columns_names[row_idx];
        DBValue **columns_to_insert_values = rows_to_insert_columns_values[row_idx];

        for (UZ column_to_insert_idx = 0;
             column_to_insert_idx < columns_to_insert_count;
             ++column_to_insert_idx) {
            StringView column_name = columns_to_insert_names[column_to_insert_idx];
            for (UZ column_idx = 0; column_idx < table->columns_count(); ++column_idx) {
                if (table->columns_names()[column_idx] != column_name) continue;

                DBValue *old_value = table_columns_values[column_idx];
                DBValue *value_to_insert = columns_to_insert_values[column_idx];
                DBValue *new_value = DBValue::concat(allocator, old_value, value_to_insert);

                table_columns_values[column_idx] = new_value;
                break;
            }
        }
    }

    table->set_rows_count(table->rows_count() + rows.count);

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
#if 0
    CHECK_WRITE(this);

    DBTable *table = table_to_update.get();
    StringView *columns_to_update_names = columns_to_update.get_items<StringView>();
    DBValue *columns_to_update_values = columns_to_update.get_items<DBValue>();

    for (UZ i = 0; i < columns_to_update.count; ++i) {
        StringView column_name = columns_to_update_names[i];
        for (UZ c = 0; c < table->columns_count(); ++c) {
            if (table->columns_names()[c] != column_name) continue;
            DBValue new_value = columns_to_update_values[i];
            table->columns_values()[c] = new_value;
            break;
        }
    }

    columns_to_update.count = 0;
    table_to_update = {};
#else
    OK_TODO();
#endif // 0
}

void QueryExecutionContext::delete_table(DBTable *table) {
#if 0
    CHECK_WRITE(this);

    for (UZ i = 0; i < table->columns_count(); ++i) {
        SQL::ColumnType column_type = table->columns_types()[i];
        SQL::Type value_type = column_type_to_type(column_type);
        DBValue new_value = DBValue::empty(value_type);
        table->columns_values()[i] = new_value;
    }

    table->set_rows_count(0);
#else
    OK_UNUSED(table);
    OK_TODO();
#endif // 0
}

void QueryExecutionContext::create_user(StringView name) {
    DBUser *user = allocator->alloc<DBUser>();
    *user = {name, ""_sv, PERM_READ | PERM_WRITE};
    current_db->add_user(user);
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

        QueryGraph::ValueNode *value_node = new (allocator) QueryGraph::ValueNode{value};
        auto *alter_node = new (allocator) QueryGraph::AlterUserNode{user_name, QueryGraph::AlterUserNode::Property::PASSWORD, value_node};
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
} // namespace xmdb
