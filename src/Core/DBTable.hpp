#pragma once

#include "BTreeIndex.hpp"
#include "ColumnLayout.hpp"
#include "DBValue.hpp"
#include "Value.hpp"
#include "ok.hpp"
#include "state.hpp"

namespace xmdb
{
/**
 * @brief Attributes of a column.
 */
struct ColumnAttribute
{
    enum
    {
        F_IMAGE = 1 << 0, ///< The column contains image data.
        F_MEDIA = 1 << 1, ///< The column contains media.
    };

    using Flags = U8;

    Flags flags; ///< The attribute flags.

    union
    {
        ImageColumnState image_state; ///< State for image columns.
    } u;
};

/**
 * @brief Gets an attribute for a column type.
 * @param column_type The column type.
 * @return The column attribute.
 */
ColumnAttribute get_attribute_for_column_type(SQL::ColumnType column_type);

/**
 * @brief Represents a database table, managing its metadata and data.
 */
class DBTable
{
public:
    using Flags = U16;
    /**
     * @brief Table flags indicating its nature and storage.
     */
    enum : Flags
    {
        F_ANON = 1 << 0, ///< Anonymous table.
        F_PERSIST = 1 << 1, ///< Persistent table (stored on disk).
        F_PROXY = 1 << 2, ///< Proxy table (values are held externally).
        F_EPHEMERAL = 1 << 3, ///< Ephemeral table (temporary).
    };

    /**
     * @brief Constructs a new DBTable.
     * @param allocator The allocator to use.
     * @param flags The table flags.
     * @param name The name of the table.
     * @param column_count The number of columns.
     * @param column_names The names of the columns.
     * @param column_types The types of the columns.
     */
    DBTable(ok::Allocator *allocator, Flags flags, ok::StringView name,
            UZ column_count, ok::StringView *column_names,
            SQL::ColumnType *column_types);

    /**
     * @brief Constructs a new DBTable.
     * @param allocator The allocator to use.
     * @param flags The table flags.
     * @param name The name of the table.
     * @param column_count The number of columns.
     * @param column_names The names of the columns.
     * @param column_types The types of the columns.
     * @param column_attributes The attributes of the columns.
     */
    DBTable(ok::Allocator *allocator, Flags flags, ok::StringView name,
            UZ column_count, ok::StringView *column_names,
            SQL::ColumnType *column_types, ColumnAttribute *column_attributes);

    /**
     * @brief Gets the table flags.
     * @return The flags.
     */
    Flags flags() const
    {
        return m_flags;
    }

    /**
     * @brief Gets the table name.
     * @return The name.
     */
    ok::StringView name() const
    {
        return m_name;
    }

    /**
     * @brief Gets the number of columns in the table.
     * @return The column count.
     */
    UZ columns_count() const
    {
        return m_columns_count;
    }

    /**
     * @brief Gets the names of the columns.
     * @return A slice of column names.
     */
    ok::Slice<ok::StringView> columns_names()
    {
        return {m_columns_names, m_columns_count};
    }

    /**
     * @brief Gets the types of the columns.
     * @return A slice of column types.
     */
    ok::Slice<SQL::ColumnType> columns_types()
    {
        return {m_columns_types, m_columns_count};
    }

    /**
     * @brief Gets the table layout.
     * @return The layout.
     */
    TableLayout layout() const
    {
        return m_layout;
    }

    /**
     * @brief Gets the total number of rows in the table.
     * @return The row count.
     */
    UZ rows_count()
    {
        if (m_flags & F_PROXY)
        {
            return m_proxy_rows_count;
        }
        else
        {
            OK_ASSERT(m_flags & F_PERSIST);
            return m_index.total_kv_count();
        }
    }

    /**
     * @brief Sets the row count for a proxy table.
     * @param x The row count.
     */
    void set_proxy_rows_count(UZ x)
    {
        OK_VERIFY(m_flags & F_PROXY);
        m_proxy_rows_count = x;
    }

    /**
     * @brief Sets the values for a proxy table's columns.
     * @param values A slice of pointers to DBValue.
     */
    void set_proxy_column_values(ok::Slice<DBValue *> values)
    {
        OK_VERIFY(m_flags & F_PROXY);
        OK_VERIFY(values.count == m_columns_count);

        for (UZ i = 0; i < m_columns_count; ++i)
        {
            SQL::Type column_type = column_type_to_type(m_columns_types[i]);
            OK_VERIFY(values[i]->type() == column_type);
        }

        m_proxy_columns_values = values.items;
    }

    /**
     * @brief Gets the column values for a proxy table.
     * @return A slice of pointers to DBValue.
     */
    ok::Slice<DBValue *> proxy_column_values()
    {
        OK_VERIFY(m_flags & F_PROXY);
        OK_VERIFY(m_proxy_columns_values != nullptr);

        return {m_proxy_columns_values, m_columns_count};
    }

    /**
     * @brief Gets the B-Tree index for a persistent table.
     * @return Pointer to the index.
     */
    BTreeIndex *index()
    {
        OK_ASSERT(m_flags & F_PERSIST);
        return &m_index;
    }

    /**
     * @brief Sets the B-Tree index for the table.
     * @param index The index to set.
     */
    void set_index(BTreeIndex index)
    {
        m_index = index;
    }

    /**
     * @brief Gets the persistence state for the table.
     * @return Pointer to the state.
     */
    DBState *state()
    {
        OK_VERIFY(m_flags & F_PERSIST);
        return &m_state;
    }

    /**
     * @brief Sets the persistence state for the table.
     * @param state The state to set.
     */
    void set_state(DBState state)
    {
        OK_VERIFY(m_flags & F_PERSIST);
        m_state = state;
    }

    /**
     * @brief Gets the attributes for each column.
     * @return A slice of column attributes.
     */
    ok::Slice<ColumnAttribute> column_attributes()
    {
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

/**
 * @brief Provides a stream-like interface for pulling values out of different
 * sources i.e. a DBTable or DBValue.
 */
class DBTableStream
{
public:
    /**
     * @brief The type of data source for the stream.
     */
    enum class Type
    {
        CONSTANT, ///< Stream from a constant value.
        COLUMN, ///< Stream from a table column.
        COMPUTE, ///< Stream from a computation function.
    };

    /**
     * @brief Constructs a DBTableStream from a constant value.
     * @param allocator The allocator to use.
     * @param constant The constant value.
     */
    explicit DBTableStream(ok::Allocator *allocator, ConstDBValue constant) :
        m_allocator{allocator},
        m_type{Type::CONSTANT},
        m_u{.constant = constant}
    {
    }

    using Computation = ok::Optional<Value> (*)(void *);

    /**
     * @brief Constructs a DBTableStream from a computation function.
     * @param allocator The allocator to use.
     * @param comp The computation function.
     * @param arg The argument to pass to the computation function.
     */
    DBTableStream(ok::Allocator *allocator, Computation comp, void *arg) :
        m_allocator{allocator},
        m_type{Type::COMPUTE},
        m_u{.compute = {.comp = comp, .arg = arg}}
    {
    }


    /**
     * @brief Constructs a DBTableStream from a table column.
     * @param allocator The allocator to use.
     * @param table The table to stream from.
     * @param column_index The index of the column to stream.
     */
    DBTableStream(ok::Allocator *allocator, DBTable *table, UZ column_index) :
        m_allocator{allocator},
        m_type{Type::COLUMN},
        m_u{.column = {
                    .table = table,
                    .column_index = column_index,
                    .current_record = 0,
                    .buffer = nullptr,
            }}
    {
        TableLayout layout = table->layout();
        ColumnLayout col_layout = layout.columns[column_index];
        m_u.column.buffer = allocator->alloc<U8>(col_layout.size);
    }


    /**
     * @brief Creates a DBTableStream from a DBValue.
     * @param allocator The allocator to use.
     * @param value The value to stream from.
     * @return The resulting stream.
     */
    static void extracted();
    static DBTableStream from_value(ok::Allocator *allocator, DBValue *value);

    /**
     * @brief Retrieves the next value from the stream.
     * @return An optional containing the next value, or empty if the stream has
     * ended.
     */
    ok::Optional<Value> next();

private:
    struct ComputationStream
    {
        Computation comp;
        void *arg;
    };

    struct ColumnStream
    {
        DBTable *table;
        UZ column_index;
        UZ current_record;
        U8 *buffer;
    };

    ok::Allocator *m_allocator;
    Type m_type;
    union
    {
        ConstDBValue constant;
        ComputationStream compute;
        ColumnStream column;
    } m_u;
};

/**
 * @brief Acts as an outlet for a DBTable, providing access to its columns as
 * streams.
 */
class DBTableOutlet
{
public:
    /**
     * @brief Constructs a new DBTableOutlet.
     * @param table The table to act as an outlet for.
     */
    explicit DBTableOutlet(DBTable *table) : m_table{table}
    {
    }

    /**
     * @brief Creates a stream for a specific column in the table.
     * @param allocator The allocator to use for the stream.
     * @param i The column index.
     * @return The resulting DBTableStream.
     */
    DBTableStream column_stream(ok::Allocator *allocator, UZ i)
    {
        ok::Slice<DBValue *> values = m_table->proxy_column_values();
        DBValue *value = values[i];
        return DBTableStream::from_value(allocator, value);
    }

private:
    DBTable *m_table;
};

static inline ok::Optional<Value> poll(ok::Allocator *allocator, DBValue *value)
{
    return DBTableStream::from_value(allocator, value).next();
}

ok::StringView table_with_media_column_dir_name(ok::Allocator *allocator,
                                                DBTable *table);

ok::StringView media_column_dir_name(ok::Allocator *allocator,
                                     ok::StringView column_name);


}; // namespace xmdb
