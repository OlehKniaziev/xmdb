#include "DBTable.hpp"

namespace xmdb {
DBTable *DBTable::alloc(ok::Allocator *allocator,
                          ok::StringView name,
                          UZ column_count,
                          ok::StringView *column_names,
                          SQL::ColumnType *column_types,
                        DBValue *column_values) {
    DBTable *table = allocator->alloc<DBTable>();
    table->name = name;
    table->column_count = column_count;
    table->column_names = column_names;
    if (column_values) {
        table->column_values = column_values;
    } else {
        table->column_values = allocator->alloc<DBValue>(column_count);

        for (UZ i = 0; i < column_count; ++i) {
            SQL::ColumnType column_type = column_types[i];
            DBValue *column_value = &table->column_values[i];

            switch (column_type) {
            case SQL::COLUMN_INTEGER: {
                TableStream<S64> ints = TableStream<S64>::empty();
                DBValue value = DBValue::integer(ints);
                *column_value = value;
                break;
            }
            case SQL::COLUMN_TEXT: {
                TableStream<String> strings = TableStream<String>::empty();
                DBValue value = DBValue::string(strings);
                *column_value = value;
                break;
            }
            default: OK_TODO();
            }
        }
    }
    table->column_types = column_types;
    table->indices = ok::Table<UZ, DBIndex>::alloc(allocator);
    return table;
}
} // namespace xmdb
