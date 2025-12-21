#pragma once

#include "DBIndex.hpp"
#include "DBValue.hpp"
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

class DBTable {
    enum : U16 {
        F_ANON,
    };

public:
    static DBTable *alloc(ok::Allocator *allocator, ok::StringView name, UZ column_count, ok::StringView *column_names,
                          SQL::ColumnType *column_types, DBValue *column_values, UZ rows_count);

    inline DBTableOutlet use() {
        return DBTableOutlet{
                .rows_count = m_rows_count,
                .values = {m_columns_values, m_columns_count},
        };
    }

    inline ok::StringView name() const {
        return m_name;
    }

    inline UZ columns_count() const {
        return m_columns_count;
    }

    inline ok::Slice<ok::StringView> columns_names() {
        return {m_columns_names, m_columns_count};
    }

    inline ok::Slice<DBValue> columns_values() {
        return {m_columns_values, m_columns_count};
    }

    inline ok::Slice<SQL::ColumnType> columns_types() {
        return {m_columns_types, m_columns_count};
    }

    inline UZ rows_count() const {
        return m_rows_count;
    }

    inline void set_rows_count(UZ x) {
        m_rows_count = x;
    }

private:
    U16 m_flags;
    ok::StringView m_name;
    UZ m_columns_count;
    UZ m_rows_count;
    ok::StringView *m_columns_names;
    SQL::ColumnType *m_columns_types;
    DBValue *m_columns_values;
    ok::Table<UZ, DBIndex> m_indices;
};
}; // namespace xmdb
