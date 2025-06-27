#include "QueryExecutionContext.hpp"

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
} // namespace xmdb
