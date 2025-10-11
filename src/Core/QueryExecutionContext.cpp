#include "QueryExecutionContext.hpp"

using namespace ok::literals;

namespace xmdb {

#define X(p)                                                                                                           \
    void CHECK_##p(QueryExecutionContext *ctx) {                                                                       \
        if ((ctx->user->perm & PERM_##p) == 0) {                                                                       \
            /* TODO(oleh): Retrieve location from the query executed. */                                               \
            ctx->error = ErrorWithSourceLocation{                                                                      \
                    .message = String::format(ctx->allocator, "user " OK_SV_FMT " does not have " #p " permissions",   \
                                              OK_SV_ARG(ctx->user->name)),                                             \
                    .location = {}};                                                                                   \
            longjmp(ctx->jmpbuf, 1);                                                                                   \
        }                                                                                                              \
    }

XMDB_ENUM_USER_PERMISSIONS

DBValue QueryExecutionContext::fetch_var(U32 ip) {
    CHECK_READ(this);
    Optional<DBValue> value = vars.get(ip);
    return value.get();
}

void QueryExecutionContext::put_var(U32 ip, StringView name, DBValue value) {
    OK_UNUSED(name);
    CHECK_WRITE(this);
    vars.put(ip, value);
}

DBValue QueryExecutionContext::fetch_column(DBTable *table, StringView column_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < table->columns_count; ++i) {
        if (table->columns_names[i] == column_name) {
            return table->columns_values[i];
        }
    }

    OK_UNREACHABLE();
}

DBTable *QueryExecutionContext::fetch_table(StringView table_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < current_db->tables.count; ++i) {
        DBTable *table = current_db->tables[i];
        if (table->name == table_name) {
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

    DBValue *column_values = nullptr;
    DBTable *new_table = DBTable::alloc(allocator, table_name, column_types.count, column_names, column_types.items,
                                        column_values, 0);
    current_db->tables.push(new_table);
}

void QueryExecutionContext::emit_column(DBTable *table, DBValue column_value, StringView column_name) {
    emitted_columns.push(column_name, column_value, table);
}

DBTable *QueryExecutionContext::emit_query(U32 columns_count, SQL::ColumnType *columns_types) {
    const StringView *columns_names_ptr = emitted_columns.get_items<StringView>();
    const DBValue *columns_values_ptr = emitted_columns.get_items<DBValue>();
    DBTable **tables = emitted_columns.get_items<DBTable *>();

    StringView *columns_names = allocator->alloc<StringView>(columns_count);
    DBValue *columns_values = allocator->alloc<DBValue>(columns_count);
    memcpy(columns_names, columns_names_ptr, columns_count * sizeof(*columns_names));
    memcpy(columns_values, columns_values_ptr, columns_count * sizeof(*columns_values));

    // FIXME(oleh): This is gonna be wrong once the row filtering is added.
    UZ rows_count = 0;

    for (UZ i = 0; i < emitted_columns.count; ++i) {
        DBTable *table = tables[i];
        if (table->rows_count > rows_count) {
            rows_count = table->rows_count;
        }
    }

    emitted_columns.count = 0;

    DBTable *table =
            DBTable::alloc(allocator, ""_sv, columns_count, columns_names, columns_types, columns_values, rows_count);
    last_emitted_query = table;
    return table;
}

void QueryExecutionContext::insert_column(DBTable *table, StringView column_name, DBValue column_value) {
    OK_UNUSED(table);
    columns_to_insert.push(column_name, column_value);
}

void QueryExecutionContext::insert_row(DBTable *table) {
    StringView *columns_names_ptr = columns_to_insert.get_items<StringView>();
    DBValue *columns_values_ptr = columns_to_insert.get_items<DBValue>();

    StringView *columns_names = allocator->alloc<StringView>(columns_to_insert.count);
    DBValue *columns_values = allocator->alloc<DBValue>(columns_to_insert.count);
    memcpy(columns_names, columns_names_ptr, sizeof(*columns_names) * columns_to_insert.count);
    memcpy(columns_values, columns_values_ptr, sizeof(*columns_values) * columns_to_insert.count);

    // FIXME(oleh): Should probably introduce a way to fetch a pointer to a value in the table,
    // and instead of inserting the list again just modify it.
    Optional<ok::MultiList<UZ, StringView *, DBValue *>> rows = rows_to_insert.get(table);
    if (rows.has_value()) {
        rows.value.push(columns_to_insert.count, columns_names, columns_values);
        rows_to_insert.put(table, rows.value);
    } else {
        ok::MultiList<UZ, StringView *, DBValue *> rows = ok::MultiList<UZ, StringView *, DBValue *>::alloc(allocator);
        rows.push(columns_to_insert.count, columns_names, columns_values);
        rows_to_insert.put(table, rows);
    }

    columns_to_insert.count = 0;
}

void QueryExecutionContext::commit_insert() {
    CHECK_WRITE(this);

    OK_TABLE_FOREACH(rows_to_insert, table, rows, {
        UZ *rows_to_insert_columns_count = rows.get_items<UZ>();
        StringView **rows_to_insert_columns_names = rows.get_items<StringView *>();
        DBValue **rows_to_insert_columns_values = rows.get_items<DBValue *>();

        for (UZ i = 0; i < rows.count; ++i) {
            UZ columns_to_insert_count = rows_to_insert_columns_count[i];
            StringView *columns_to_insert_names = rows_to_insert_columns_names[i];
            DBValue *columns_to_insert_values = rows_to_insert_columns_values[i];

            for (UZ j = 0; j < columns_to_insert_count; ++j) {
                StringView column_name = columns_to_insert_names[j];
                for (UZ c = 0; c < table->columns_count; ++c) {
                    if (table->columns_names[c] != column_name) continue;

                    DBValue column_value = columns_to_insert_values[j];
                    DBValue old_value = table->columns_values[c];
                    DBValue new_value = DBValue::concat(allocator, old_value, column_value);

                    table->columns_values[c] = new_value;
                    break;
                }
            }
        }

        table->rows_count += rows.count;
    });

    rows_to_insert.clear();
}

void QueryExecutionContext::update_column(DBTable *table, StringView column_name, DBValue column_value) {
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
    DBValue *columns_to_update_values = columns_to_update.get_items<DBValue>();

    for (UZ i = 0; i < columns_to_update.count; ++i) {
        StringView column_name = columns_to_update_names[i];
        for (UZ c = 0; c < table->columns_count; ++c) {
            if (table->columns_names[c] != column_name) continue;
            DBValue new_value = columns_to_update_values[i];
            table->columns_values[c] = new_value;
            break;
        }
    }

    columns_to_update.count = 0;
    table_to_update = {};
}

void QueryExecutionContext::delete_table(DBTable *table) {
    CHECK_WRITE(this);

    for (UZ i = 0; i < table->columns_count; ++i) {
        SQL::ColumnType column_type = table->columns_types[i];
        SQL::Type value_type = column_type_to_type(column_type);
        DBValue new_value = DBValue::empty(value_type);
        table->columns_values[i] = new_value;
    }

    table->rows_count = 0;
}

void QueryExecutionContext::create_user(StringView name) {
    DBUser *user = allocator->alloc<DBUser>();
    *user = {name, ""_sv, PERM_READ | PERM_WRITE};
    current_db->add_user(user);
}
} // namespace xmdb
