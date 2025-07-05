#include "QueryExecutionContext.hpp"

using namespace ok::literals;

namespace xmdb {
DBValue QueryExecutionContext::fetch_var(U32 ip) {
    Optional<DBValue> value = vars.get(ip);
    return value.get();
}

void QueryExecutionContext::put_var(U32 ip, StringView name, DBValue value) {
    OK_UNUSED(name);
    vars.put(ip, value);
}

DBValue QueryExecutionContext::fetch_column(DBTable *table, StringView column_name) {
    for (UZ i = 0; i < table->column_count; ++i) {
        if (table->column_names[i] == column_name) {
            return table->column_values[i];
        }
    }

    OK_UNREACHABLE();
}

DBTable *QueryExecutionContext::fetch_table(StringView table_name) {
    for (UZ i = 0; i < current_db->tables.count; ++i) {
        DBTable *table = current_db->tables[i];
        if (table->name == table_name) {
            return table;
        }
    }

    OK_UNREACHABLE();
}

void QueryExecutionContext::create_table(StringView table_name, SQL::TableSchema *table_schema) {
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
    DBTable *new_table = DBTable::alloc(allocator,
                                        table_name,
                                        column_types.count,
                                        column_names,
                                        column_types.items,
                                        column_values);
    current_db->tables.push(new_table);
}

void QueryExecutionContext::emit_column(DBValue column_value, StringView column_name) {
    emitted_columns.push(column_name, column_value);
}

DBTable *QueryExecutionContext::emit_query(U32 columns_count, SQL::ColumnType *columns_types) {
    emitted_columns.count -= columns_count;
    const StringView *columns_names_ptr = emitted_columns.get_items<StringView>();
    const DBValue *columns_values_ptr = emitted_columns.get_items<DBValue>();

    StringView *columns_names = allocator->alloc<StringView>(columns_count);
    DBValue *columns_values = allocator->alloc<DBValue>(columns_count);
    memcpy(columns_names, columns_names_ptr, columns_count * sizeof(*columns_names));
    memcpy(columns_values, columns_values_ptr, columns_count * sizeof(*columns_values));

    DBTable *table = DBTable::alloc(allocator,
                                    ""_sv,
                                    columns_count,
                                    columns_names,
                                    columns_types,
                                    columns_values);
    last_emitted_query = table;
    return table;
}
} // namespace xmdb
