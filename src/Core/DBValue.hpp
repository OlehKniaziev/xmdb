#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"

namespace xmdb {
class DBValue {
public:
    enum class Kind {
        COLUMN,
        // WRITE,
        COMPARE,
        CONSTANT,
        // ALTER_USER,
        // ATOMIC,
    };

    DBValue() = delete;

    static DBValue *concat(ok::Allocator *, DBValue *, DBValue *) {
        OK_TODO();
    }

    static DBValue *null() {
        OK_TODO();
    }

    SQL::Type type() const {
        return m_type;
    }

    Kind kind() const {
        return m_kind;
    }

protected:
    explicit DBValue(SQL::Type type, Kind kind) : m_type{type}, m_kind{kind} {}

    SQL::Type m_type;
    Kind m_kind;
};

class ColumnDBValue : public DBValue {
public:
    ColumnDBValue(SQL::Type type, ColumnLayout layout) : DBValue{type, Kind::COLUMN}, m_layout{layout} {}

private:
    ColumnLayout m_layout;
};

class ConstDBValue : public DBValue {
public:
    enum class ConstKind {
        INT,
        STRING,
        BOOL,
    };

    explicit ConstDBValue(S64 i) : ConstDBValue{SQL::TYPE_INT, ConstKind::INT, reinterpret_cast<void *>(i)} {}
    explicit ConstDBValue(ok::String *s) : ConstDBValue{SQL::TYPE_STRING, ConstKind::STRING, static_cast<void *>(s)} {}
    explicit ConstDBValue(bool b) : ConstDBValue{SQL::TYPE_BOOL, ConstKind::BOOL, reinterpret_cast<void *>(static_cast<U64>(b))} {}

private:
    explicit ConstDBValue(SQL::Type type, ConstKind kind, void *data) : DBValue{type, Kind::CONSTANT}, m_const_kind{kind}, m_data{data} {}

    ConstKind m_const_kind;
    void *m_data;
};

class CompareDBValue : public DBValue {
public:
    CompareDBValue(DBValue *lhs, DBValue *rhs) : DBValue{lhs->type(), Kind::COMPARE}, m_lhs{lhs}, m_rhs{rhs} {
        OK_ASSERT(lhs->type() == rhs->type());
    }

    DBValue *lhs() {
        return m_lhs;
    }

    DBValue *rhs() {
        return m_rhs;
    }

private:
    DBValue *m_lhs;
    DBValue *m_rhs;
};
} // namespace xmdb
