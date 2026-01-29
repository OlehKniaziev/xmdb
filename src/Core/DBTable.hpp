#pragma once

#include "BTreeIndex.hpp"
#include "DBValue.hpp"
#include "ColumnLayout.hpp"
#include "ok.hpp"
#include "Value.hpp"
#include "state.hpp"

namespace xmdb {
struct ColumnAttribute {
    enum {
        F_IMAGE = 1 << 0,
    };

    using Flags = U8;

    Flags flags;

    union {
        ImageColumnState image_state;
    } u;
};

class DBTable {
public:
    using Flags = U16;
    enum : Flags {
        F_ANON      = 1 << 0,
        F_PERSIST   = 1 << 1,
        F_PROXY     = 1 << 2,
        F_EPHEMERAL = 1 << 3,
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

    TableLayout layout() const {
        return m_layout;
    }

    UZ rows_count() {
        if (m_flags & F_PROXY) {
            return m_proxy_rows_count;
        } else {
            OK_ASSERT(m_flags & F_PERSIST);
            return m_index.node_count();
        }
    }

    void set_proxy_rows_count(UZ x) {
        OK_VERIFY(m_flags & F_PROXY);
        m_proxy_rows_count = x;
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

    BTreeIndex *index() {
        OK_ASSERT(m_flags & F_PERSIST);
        return &m_index;
    }

    void set_index(BTreeIndex index) {
        m_index = index;
    }

    DBState *state() {
        OK_VERIFY(m_flags & F_PERSIST);
        return &m_state;
    }

    void set_state(DBState state) {
        OK_VERIFY(m_flags & F_PERSIST);
        m_state = state;
    }

    ok::Slice<ColumnAttribute> column_attributes() {
        return {m_column_attributes, m_columns_count};
    }

private:
    ok::StringView m_name;
    U16 m_flags;
    UZ m_columns_count;
    ok::StringView *m_columns_names;
    SQL::ColumnType *m_columns_types;
    TableLayout m_layout;
    DBValue **m_proxy_columns_values;
    ColumnAttribute *m_column_attributes;
    UZ m_proxy_rows_count;
    BTreeIndex m_index;
    DBState m_state;
};

class DBTableStream {
public:
    enum class Type {
        CONSTANT,
        COLUMN,
        COMPUTE,
    };

    explicit DBTableStream(ok::Allocator *allocator, ConstDBValue constant) : m_allocator{allocator}, m_type{Type::CONSTANT}, m_u{.constant = constant} {}

    using Computation = ok::Optional<Value> (*)(void *);

    DBTableStream(ok::Allocator *allocator, Computation comp, void *arg) : m_allocator{allocator}, m_type{Type::COMPUTE}, m_u{.compute = {.comp = comp, .arg = arg}} {}

    DBTableStream(ok::Allocator *allocator,
                  DBTable *table,
                  UZ column_index) : m_allocator{allocator},
                                     m_type{Type::COLUMN},
                                     m_u{
                                         .column = {
                                             .table = table,
                                             .column_index = column_index,
                                             .current_record = 0,
                                             .buffer = nullptr,
                                         }
                                     }
    {
        TableLayout layout = table->layout();
        ColumnLayout col_layout = layout.columns[column_index];
        m_u.column.buffer = allocator->alloc<U8>(col_layout.size);
    }

    static DBTableStream from_value(ok::Allocator *, DBValue *);

    ok::Optional<Value> next();

private:
    struct ComputationStream {
        Computation comp;
        void *arg;
    };

    struct ColumnStream {
        DBTable *table;
        UZ column_index;
        UZ current_record;
        U8 *buffer;
    };

    ok::Allocator *m_allocator;
    Type m_type;
    union {
        ConstDBValue constant;
        ComputationStream compute;
        ColumnStream column;
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

static inline ok::Optional<Value> poll(ok::Allocator *allocator, DBValue *value) {
    return DBTableStream::from_value(allocator, value).next();
}
}; // namespace xmdb
