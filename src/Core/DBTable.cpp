#include "DBTable.hpp"
#include "FixedString.hpp"

namespace xmdb {
template <>
S64 Value::cast<S64>() {
    OK_ASSERT(m_type == SQL::TYPE_INT);
    return static_cast<S64>(reinterpret_cast<U64>(m_data));
}

template <>
ok::String Value::cast<ok::String>() {
    OK_ASSERT(m_type == SQL::TYPE_STRING);
    return *reinterpret_cast<ok::String *>(m_data);
}

template <>
bool Value::cast<bool>() {
    OK_ASSERT(m_type == SQL::TYPE_BOOL);
    return static_cast<bool>(reinterpret_cast<U64>(m_data));
}

struct TypeLayout {
    static constexpr UZ MAX_ALIGNMENT = sizeof(UZ);

    UZ size;
    UZ alignment;
};

static TypeLayout type_layout(SQL::ColumnType type) {
    static_assert(FixedString::SIZE % 8 == 0);

    switch (type) {
    case SQL::ColumnType::INTEGER: return {.size = 8, .alignment = 8};
    case SQL::ColumnType::FLOAT:   return {.size = 4, .alignment = 4};
    case SQL::ColumnType::DOUBLE:  return {.size = 8, .alignment = 8};
    case SQL::ColumnType::BOOLEAN: return {.size = 1, .alignment = 1};
    case SQL::ColumnType::TEXT:    return {.size = FixedString::SIZE, .alignment = 8};
    case SQL::ColumnType::IMAGE:   OK_TODO();
    }

    OK_UNREACHABLE();
}

// TODO(oleh): Reorder columns so that disk layout is optimized. Usually this would involve sorting the columns by type
// sizes in ascending order.
static ColumnLayout *compute_columns_layout(ok::Allocator *allocator,
                                            UZ columns_count,
                                            SQL::ColumnType *columns_types) {
    ColumnLayout *columns_layout = allocator->alloc<ColumnLayout>(columns_count);
    UZ offset = 0;

    for (UZ i = 0; i < columns_count; ++i) {
        SQL::ColumnType column_type = columns_types[i];
        TypeLayout t_layout = type_layout(column_type);

        if (t_layout.alignment != 1) {
            OK_ASSERT((t_layout.alignment & 1) == 0);

            offset = ok::align_up(offset, t_layout.alignment);
        }

        ColumnLayout column_layout = {
            .offset = offset,
            .size = t_layout.size,
        };

        columns_layout[i] = column_layout;

        offset += t_layout.size;
    }

    return columns_layout;
}

DBTable::DBTable(ok::Allocator *allocator,
                 DBTable::Flags flags,
                 ok::StringView name,
                 UZ columns_count,
                 ok::StringView *columns_names,
                 SQL::ColumnType *columns_types) : m_name{name}, m_flags{flags}, m_columns_count{columns_count}, m_columns_names{columns_names}, m_columns_types{columns_types} {
    m_indices = ok::Table<UZ, DBIndex>::alloc(allocator);
    m_columns_layout = compute_columns_layout(allocator, columns_count, columns_types);
}
} // namespace xmdb
