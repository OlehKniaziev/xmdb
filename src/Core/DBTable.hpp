#pragma once

#include "DBValue.hpp"
#include "DBIndex.hpp"
#include "ok.hpp"

namespace xmdb {
struct DBTableOutlet {
    ~DBTableOutlet() {
        for (UZ i = 0; i < values.count; ++i) {
            values[i].reset();
        }
    }

    UZ rows_count;
    ok::Slice<DBValue> values;
};

struct DBTable {
    enum : U16 {
        F_ANON,
    };

    static DBTable *alloc(ok::Allocator *allocator,
                          ok::StringView name,
                          UZ column_count,
                          ok::StringView *column_names,
                          SQL::ColumnType *column_types,
                          DBValue *column_values,
                          UZ rows_count);

    inline DBTableOutlet use() {
        return DBTableOutlet {
            .rows_count = rows_count,
            .values = {columns_values, columns_count},
        };
    }

    U16 flags;
    ok::StringView name;
    UZ columns_count;
    UZ rows_count;
    ok::StringView *columns_names;
    SQL::ColumnType *columns_types;
    DBValue *columns_values;
    ok::Table<UZ, DBIndex> indices;
};
};
