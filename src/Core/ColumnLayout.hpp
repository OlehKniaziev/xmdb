#pragma once

namespace xmdb
{
/**
 * @brief Layout information for a single column in a table record.
 */
struct ColumnLayout
{
    U64 index; ///< The index of the column.
    U64 offset; ///< The byte offset of the column within the record.
    U64 size; ///< The size of the column in bytes.
};

/**
 * @brief Layout information for an entire table record.
 */
struct TableLayout
{
    UZ primary_key_index; ///< The index of the primary key column.
    ok::Slice<ColumnLayout> columns; ///< The collection of column layouts.
};

static inline UZ table_record_size(TableLayout layout)
{
    OK_ASSERT(layout.columns.count > 0);
    ColumnLayout last = layout.columns[layout.columns.count - 1];
    return last.offset + last.size;
}
} // namespace xmdb
