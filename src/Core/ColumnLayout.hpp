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
}
