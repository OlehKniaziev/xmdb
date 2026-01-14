#pragma once

#include "DBIndex.hpp"
#include "DBValue.hpp"
#include "ColumnLayout.hpp"
#include "ok.hpp"

namespace xmdb {
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

    void set_proxy_column_values(ok::Slice<DBValue *> values) {
        OK_VERIFY(m_flags & F_PROXY);
        OK_VERIFY(values.count == m_columns_count);

        for (UZ i = 0; i < m_columns_count; ++i) {
            SQL::Type column_type = column_type_to_type(m_columns_types[i]);
            OK_VERIFY(values[i]->type() == column_type);
        }

        m_proxy_columns_values = values.items;
    }

    ok::Slice<DBValue *> proxy_column_values() {
        OK_VERIFY(m_flags & F_PROXY);
        OK_VERIFY(m_proxy_columns_values != nullptr);

        return {m_proxy_columns_values, m_columns_count};
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
    DBValue **m_proxy_columns_values; // NOTE(oleh): I just don't fucking care anymore.
};

class Value {
public:
    Value() = delete;

    static Value integer(S64 value) {
        return Value{SQL::TYPE_INT, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    static Value greater() {
        return integer(1);
    }

    static Value less() {
        return integer(-1);
    }

    static Value equal() {
        return integer(0);
    }

    template <typename T>
    T cast() = delete;

    SQL::Type type() const {
        return m_type;
    }

    Value compare(Value);

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
    enum class Type {
        CONSTANT,
        DISK,
        COMPUTE,
    };

    explicit DBTableStream(ConstDBValue *constant) : m_type{Type::CONSTANT}, m_u{.constant = constant} {}

    using Computation = ok::Optional<Value> (*)(void *);

    explicit DBTableStream(Computation comp, void *arg) : m_type{Type::COMPUTE}, m_u{.compute = {.comp = comp, .arg = arg}} {}

    static DBTableStream from_value(ok::Allocator *, DBValue *);

    ok::Optional<Value> next();

private:
    struct ComputationStream {
        Computation comp;
        void *arg;
    };

    Type m_type;
    union {
        ConstDBValue *constant;
        ComputationStream compute;
    } m_u;
};

class DBTableOutlet {
public:
    explicit DBTableOutlet(DBTable *table) : m_table{table} {}

    DBTableStream column_stream(ok::Allocator *allocator, UZ i) {
        ok::Slice<DBValue *> values = m_table->proxy_column_values();
        DBValue *value = values[i];
        return DBTableStream::from_value(allocator, value);
    }

private:
    DBTable *m_table;
};
}; // namespace xmdb
