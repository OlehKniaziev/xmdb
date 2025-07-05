#pragma once

#include "DBValue.hpp"
#include "DBIndex.hpp"
#include "ok.hpp"

namespace xmdb {
struct DBTable {
    enum : U16 {
        F_ANON,
    };

    static DBTable *alloc(ok::Allocator *allocator,
                          ok::StringView name,
                          UZ column_count,
                          ok::StringView *column_names,
                          SQL::ColumnType *column_types,
                          DBValue *column_values);

    U16 flags;
    ok::StringView name;
    UZ column_count;
    ok::StringView *column_names;
    SQL::ColumnType *column_types;
    DBValue *column_values;
    ok::Table<UZ, DBIndex> indices;
};
};
