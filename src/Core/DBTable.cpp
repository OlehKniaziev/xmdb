#include "DBTable.hpp"
#include "FixedString.hpp"
#include "new.hpp"
#include "Logger.hpp"
#include "constants.hpp"

using namespace ok::literals;

namespace xmdb {
struct TypeLayout {
    static constexpr UZ MAX_ALIGNMENT = sizeof(UZ);

    UZ size;
    UZ alignment;
};

static TypeLayout type_layout(SQL::ColumnType type) {
    static_assert(sizeof(FixedString) % 8 == 0);

    switch (type) {
    case SQL::ColumnType::INTEGER: return {.size = 8, .alignment = 8};
    case SQL::ColumnType::FLOAT:   return {.size = 4, .alignment = 4};
    case SQL::ColumnType::DOUBLE:  return {.size = 8, .alignment = 8};
    case SQL::ColumnType::BOOLEAN: return {.size = 1, .alignment = 1};
    case SQL::ColumnType::TEXT:    return {.size = sizeof(FixedString), .alignment = 8};
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
    ColumnLayout *columns_layout = compute_columns_layout(allocator, columns_count, columns_types);

    UZ id_column_index = (UZ)-1;
    for (UZ i = 0; i < columns_count; ++i) {
        if (columns_names[i] == "id"_sv) {
            id_column_index = i;
            break;
        }
    }

    if (id_column_index == (UZ)-1) {
        OK_VERIFY(columns_count > 0);
        id_column_index = 0;
    }

    XMDB_FIXME("DBTable constructor searches for a column with name 'id' and sets it as a primary key column");

    m_layout.primary_key_index = id_column_index;
    m_layout.columns.items = columns_layout;
    m_layout.columns.count = columns_count;

    if (flags & F_PROXY) {
        OK_ASSERT(flags & F_ANON);

        m_proxy_columns_values = allocator->alloc<DBValue *>(m_columns_count);
        for (UZ i = 0; i < m_columns_count; ++i) {
            m_proxy_columns_values[i] = new (allocator) NoneDBValue{};
        }
    }

    if (flags & F_EPHEMERAL) {
        OK_ASSERT(flags & F_PERSIST);
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
        return DBTableStream{allocator, *const_val};
    }
    case DBValue::Kind::COMPARE: {
        auto *cmp_val = static_cast<CompareDBValue *>(value);

        DBTableStream lhs = from_value(allocator, cmp_val->lhs());
        DBTableStream rhs = from_value(allocator, cmp_val->rhs());

        auto *cmp_state = new (allocator) StreamPair{lhs, rhs};

        return DBTableStream{
            allocator,
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
            allocator,
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
            allocator,
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
        ColumnLayout column_layout = column_value->layout();

        if (table->flags() & DBTable::F_PROXY) {
            ok::Slice<DBValue *> columns_values = table->proxy_column_values();

            DBValue *value = columns_values[column_layout.index];

            return DBTableStream::from_value(allocator, value);
        } else {
            OK_ASSERT(table->flags() & DBTable::F_PERSIST);

            return DBTableStream{
                allocator,
                table,
                column_layout.index,
            };
        }
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
            OK_VERIFY(value->count() <= FixedString::DATA_SIZE);
            FixedString fs{};
            memcpy(fs.items, (U8 *) value->cstr(), value->count());
            fs.count = (U8) value->count();
            return Value::string(m_allocator, fs);
        }
        }

        OK_UNREACHABLE();
    }
    case Type::COMPUTE: {
        ComputationStream comp = m_u.compute;
        return comp.comp(comp.arg);
    }
    case Type::COLUMN: {
        ColumnStream *col = &m_u.column;
        DBTable *table = col->table;
        DBState *state = table->state();
        UZ records_count = state->header.record_count;

        if (col->current_record >= records_count) {
            return ok::Optional<Value>::empty();
        }

        TableLayout layout = table->layout();
        ColumnLayout target_column = layout.columns[col->column_index];

        OK_ASSERT(layout.columns.count > 0);
        ColumnLayout last = layout.columns[layout.columns.count - 1];
        UZ record_size = last.offset + last.size;

        state->file.seek_to(DB_STATE_HEADER_LENGTH + col->current_record * record_size + target_column.offset);

        UZ n_read = 0;

        ok::Optional<ok::File::ReadError> read_err = state->file.read(col->buffer, target_column.size, &n_read);
        OK_VERIFY(!read_err);
        OK_VERIFY(n_read == target_column.size);

        SQL::ColumnType column_type = table->columns_types()[col->column_index];

        ++col->current_record;

        switch (column_type) {
        case SQL::ColumnType::INTEGER: {
            U64 result_u = 0;

            for (SZ i = 0; i < 7; ++i) {
                result_u = (result_u | (U64) col->buffer[i]) << 8;
            }

            result_u |= (U64) col->buffer[7];

            S64 result = (S64) result_u;

            return Value::integer(result);
        }
        case SQL::ColumnType::TEXT: {
            FixedString result{};
            memcpy(&result, col->buffer, sizeof(FixedString));
            return Value::string(m_allocator, result);
        }
        case SQL::ColumnType::BOOLEAN:
            OK_TODO_MSG("[next] BOOL");
        case SQL::ColumnType::FLOAT:
            OK_TODO_MSG("[next] FLOAT");
        case SQL::ColumnType::DOUBLE:
            OK_TODO_MSG("[next] DOUBLE");
        case SQL::ColumnType::IMAGE:
            OK_TODO_MSG("[next] IMAGE");
        }

        OK_UNREACHABLE();
    }
    }

    OK_UNREACHABLE();
}

Value Value::compare(Value) {
    OK_TODO();
}
} // namespace xmdb
