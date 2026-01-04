#pragma once

#include "DBIndex.hpp"
#include "DBValue.hpp"
#include "ColumnLayout.hpp"
#include "ok.hpp"

namespace xmdb {
class Value {
public:
    Value() = delete;

    static Value integer(S64 value) {
        return Value{SQL::TYPE_INT, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    template <typename T>
    T cast() = delete;

    SQL::Type type() const {
        return m_type;
    }

private:
    Value(SQL::Type type, void *data) : m_type{type}, m_data{data} {}

    SQL::Type m_type;
    void *m_data;
};

template <>
S64 Value::cast<S64>();

template <>
ok::String Value::cast<ok::String>();

template <>
bool Value::cast<bool>();

class DBTableStream {
public:
    ok::Optional<Value> next() {
        OK_TODO();
    }

private:
};

class DBTable {
public:
    using Flags = U16;
    enum : Flags {
        F_ANON    = 1 << 0,
        F_PERSIST = 1 << 1,
        F_PROXY   = 1 << 2,
    };

    DBTable(ok::Allocator *allocator,
            Flags flags,
            ok::StringView name,
            UZ column_count,
            ok::StringView *column_names,
            SQL::ColumnType *column_types);

    Flags flags() const {
        return m_flags;
    }

    ok::StringView name() const {
        return m_name;
    }

    UZ columns_count() const {
        return m_columns_count;
    }

    ok::Slice<ok::StringView> columns_names() {
        return {m_columns_names, m_columns_count};
    }

    ok::Slice<SQL::ColumnType> columns_types() {
        return {m_columns_types, m_columns_count};
    }

    ok::Slice<ColumnLayout> columns_layout() {
        return {m_columns_layout, m_columns_count};
    }

    UZ rows_count() const {
        return m_rows_count;
    }

    void set_rows_count(UZ x) {
        m_rows_count = x;
    }

protected:
    ok::StringView m_name;
    U16 m_flags;
    UZ m_columns_count;
    UZ m_rows_count;
    ok::StringView *m_columns_names;
    SQL::ColumnType *m_columns_types;
    ok::Table<UZ, DBIndex> m_indices;
    ColumnLayout *m_columns_layout;
};

class DBTableOutlet {
public:
    explicit DBTableOutlet(DBTable &table) : m_table{table} {}

    DBTableStream column_stream(UZ) {
        OK_TODO();
    }

private:
    DBTable &m_table;
};
}; // namespace xmdb
