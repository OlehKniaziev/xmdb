#include "DBRecord.hpp"
#include "FixedString.hpp"

namespace xmdb {
void fill_column(DBRecord *record,
                 TableLayout table_layout,
                 UZ column_index,
                 Value column_value) {
    OK_VERIFY(column_index < table_layout.columns.count);

    ColumnLayout column_layout = table_layout.columns[column_index];
    OK_ASSERT(column_layout.index == column_index);

    OK_VERIFY(column_layout.offset + column_layout.size <= record->buffer_size);

    switch (column_value.type()) {
    case SQL::TYPE_INT: {
        S64 i = column_value.as_int();
        OK_VERIFY(sizeof(i) == column_layout.size);

        UZ o = column_layout.offset;

        for (SZ bit_offset = 56; bit_offset >= 0; bit_offset -= 8) {
            U8 b = ((U64) i >> bit_offset) & 0xFF;
            record->buffer[o] = b;
            ++o;
        }

        if (column_index == table_layout.primary_key_index) {
            record->primary_key_value = (U64)i;
        }

        break;
    }
    case SQL::TYPE_BOOL: {
        bool b = column_value.as_bool();
        OK_VERIFY(sizeof(b) == 1);
        OK_VERIFY(sizeof(b) == column_layout.size);

        record->buffer[column_layout.offset] = b;
        break;
    }
    case SQL::TYPE_STRING: {
        FixedString fs = column_value.as_string();

        OK_VERIFY(sizeof(fs) == column_layout.size);

        memcpy(&record->buffer[column_layout.offset], reinterpret_cast<U8 *>(&fs), sizeof(fs));

        break;
    }
    case SQL::TYPE_PNG:
        OK_TODO_MSG("TYPE_PNG");
    case SQL::TYPE_NULL:
        OK_TODO_MSG("TYPE_NULL");
    case SQL::TYPE_FLOAT:
        OK_TODO_MSG("TYPE_FLOAT");
    case SQL::TYPE_DOUBLE:
        OK_TODO_MSG("TYPE_DOUBLE");
    case SQL::TYPE_TABLE:
        OK_TODO_MSG("TYPE_TABLE");
    }
}

DBRecord db_record_create(ok::Allocator *allocator, TableLayout table_layout) {
    ColumnLayout prev_column_layout{};

    UZ total_record_size = 0;

    for (UZ i = 0; i < table_layout.columns.count; ++i) {
        // NOTE(oleh): Check that there are no holes or overlaps.
        ColumnLayout column_layout = table_layout.columns[i];
        OK_ASSERT(column_layout.offset == prev_column_layout.offset + prev_column_layout.size);
        prev_column_layout = column_layout;

        total_record_size += column_layout.size;
    }

    UZ buffer_size = total_record_size;
    U8 *buffer = allocator->alloc<U8>(buffer_size);
    return DBRecord{
        .buffer = buffer,
        .buffer_size = buffer_size,
        .primary_key_value = 0,
    };
}
} // namespace xmdb
