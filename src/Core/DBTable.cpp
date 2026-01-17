#include "DBTable.hpp"
#include "FixedString.hpp"
#include "new.hpp"

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
            .index = i,
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
                 SQL::ColumnType *columns_types) : m_name{name},
                                                   m_flags{flags},
                                                   m_columns_count{columns_count},
                                                   m_columns_names{columns_names},
                                                   m_columns_types{columns_types} {
    m_indices = ok::Table<UZ, DBIndex>::alloc(allocator);
    m_columns_layout = compute_columns_layout(allocator, columns_count, columns_types);

    if (flags & F_PROXY) {
        OK_ASSERT(flags & F_ANON);

        m_proxy_columns_values = allocator->alloc<DBValue *>(m_columns_count);
        for (UZ i = 0; i < m_columns_count; ++i) {
            m_proxy_columns_values[i] = new (allocator) NoneDBValue{};
        }
    }
}

struct StreamPair {
    DBTableStream lhs;
    DBTableStream rhs;
};

DBTableStream DBTableStream::from_value(ok::Allocator *allocator, DBValue *value) {
    switch (value->kind()) {
    case DBValue::Kind::CONSTANT: {
        auto *const_val = static_cast<ConstDBValue *>(value);
        return DBTableStream{*const_val};
    }
    case DBValue::Kind::COMPARE: {
        auto *cmp_val = static_cast<CompareDBValue *>(value);

        DBTableStream lhs = from_value(allocator, cmp_val->lhs());
        DBTableStream rhs = from_value(allocator, cmp_val->rhs());

        auto *cmp_state = new (allocator) StreamPair{lhs, rhs};

        return DBTableStream{
            [](void *arg) -> ok::Optional<Value> {
                auto *state = static_cast<StreamPair *>(arg);
                ok::Optional<Value> lhs_opt = state->lhs.next();
                ok::Optional<Value> rhs_opt = state->rhs.next();

                if (lhs_opt && rhs_opt) {
                    Value lhs = lhs_opt.get();
                    Value rhs = rhs_opt.get();
                    return lhs.compare(rhs);
                }

                if (lhs_opt) {
                    return Value::greater();
                }

                if (rhs_opt) {
                    return Value::less();
                }

                return ok::Optional<Value>::empty();
            },
            static_cast<void *>(cmp_state),
        };
    }
    case DBValue::Kind::NONE: {
        return DBTableStream{
            [](void *arg) -> ok::Optional<Value> {
                OK_UNUSED(arg);
                return ok::Optional<Value>::empty();
            },
            nullptr,
        };
    }
    case DBValue::Kind::CONCAT: {
        auto *concat_value = static_cast<ConcatDBValue *>(value);

        DBTableStream lhs = from_value(allocator, concat_value->lhs());
        DBTableStream rhs = from_value(allocator, concat_value->rhs());

        auto *stream_pair = new (allocator) StreamPair{lhs, rhs};

        return DBTableStream{
            [](void *arg) -> ok::Optional<Value> {
                auto *pair = reinterpret_cast<StreamPair *>(arg);

                if (ok::Optional<Value> lhs = pair->lhs.next(); lhs) {
                    return lhs;
                } else {
                    return pair->rhs.next();
                }
            },
            stream_pair,
        };
    }
    case DBValue::Kind::COLUMN: {
        auto *column_value = static_cast<ColumnDBValue *>(value);
        DBTable *table = column_value->table();

        ok::Slice<DBValue *> columns_values = table->proxy_column_values();

        ColumnLayout column_layout = column_value->layout();

        DBValue *value = columns_values[column_layout.index];

        return DBTableStream::from_value(allocator, value);
    }
    }

    OK_UNREACHABLE();
}

ok::Optional<Value> DBTableStream::next() {
    switch (m_type) {
    case Type::CONSTANT: {
        ConstDBValue constant = m_u.constant;

        switch (constant.kind()) {
        case ConstDBValue::ConstKind::INT: {
            S64 value = constant.as_int();
            return Value::integer(value);
        }
        case ConstDBValue::ConstKind::BOOL: {
            bool value = constant.as_bool();
            return Value::boolean(value);
        }
        case ConstDBValue::ConstKind::STRING: {
            ok::String *value = constant.as_string();
            return Value::string(value);
        }
        }

        OK_UNREACHABLE();
    }
    case Type::COMPUTE: {
        ComputationStream comp = m_u.compute;
        return comp.comp(comp.arg);
    }
    case Type::DISK:
        OK_TODO_MSG("Disk table stream. FUCK!");
    }

    OK_UNREACHABLE();
}

Value Value::compare(Value) {
    OK_TODO();
}
} // namespace xmdb
