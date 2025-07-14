#include "DBTable.hpp"

namespace xmdb {
DBTable *DBTable::alloc(ok::Allocator *allocator,
                        ok::StringView name,
                        UZ columns_count,
                        ok::StringView *columns_names,
                        SQL::ColumnType *columns_types,
                        DBValue *columns_values,
                        UZ rows_count) {
    DBTable *table = allocator->alloc<DBTable>();
    table->name = name;
    table->columns_count = columns_count;
    table->columns_names = columns_names;
    if (columns_values != nullptr) {
        table->columns_values = columns_values;
        table->rows_count = rows_count;
    } else {
        table->columns_values = allocator->alloc<DBValue>(columns_count);

        for (UZ i = 0; i < columns_count; ++i) {
            SQL::ColumnType column_type = columns_types[i];
            DBValue *column_value = &table->columns_values[i];

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

        table->rows_count = 0;
    }
    table->columns_types = columns_types;
    table->indices = ok::Table<UZ, DBIndex>::alloc(allocator);
    return table;
}
} // namespace xmdb
