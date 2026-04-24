#include "DBRecord.hpp"
#include "FixedString.hpp"

namespace xmdb
{
DBRecord db_record_create(ok::Allocator *allocator, TableLayout table_layout)
{
    ColumnLayout prev_column_layout{};

    UZ total_record_size = 0;

    for (UZ i = 0; i < table_layout.columns.count; ++i)
    {
        // NOTE(oleh): Check that there are no holes or overlaps.
        ColumnLayout column_layout = table_layout.columns[i];
        OK_ASSERT(column_layout.offset ==
                  prev_column_layout.offset + prev_column_layout.size);
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
