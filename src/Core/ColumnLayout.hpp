#pragma once

namespace xmdb {
struct ColumnLayout {
    U64 index;
    U64 offset;
    U64 size;
};

struct TableLayout {
    UZ primary_key_index;
    ok::Slice<ColumnLayout> columns;
};

static inline UZ table_record_size(TableLayout layout) {
    OK_ASSERT(layout.columns.count > 0);
    ColumnLayout last = layout.columns[layout.columns.count - 1];
    return last.offset + last.size;
}
} // namespace xmdb
