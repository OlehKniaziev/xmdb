#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"
#include "image.hpp"

namespace xmdb {
class DBValue {
public:
    enum class Kind {
        COLUMN,
        COMPARE,
        CONSTANT,
        NONE,
        CONCAT,
        IMAGE_DATA,
    };

    DBValue() = delete;

    static DBValue *null() {
        OK_TODO();
    }

    SQL::Type type() const {
        return m_type;
    }

    Kind kind() const {
        return m_kind;
    }

    bool is_compatible_with(const DBValue *other) const {
        if (m_kind == Kind::NONE || other->m_kind == Kind::NONE) {
            return true;
        }

        return m_type == other->m_type;
    }

protected:
    DBValue(SQL::Type type, Kind kind) : m_type{type}, m_kind{kind} {}

    SQL::Type m_type;
    Kind m_kind;
};

class DBTable;

class ColumnDBValue : public DBValue {
public:
    ColumnDBValue(SQL::Type type,
                  ColumnLayout layout,
                  DBTable *table) : DBValue{type, Kind::COLUMN},
                                    m_layout{layout},
                                    m_table{table} {}

    DBTable *table() {
        return m_table;
    }

    ColumnLayout layout() {
        return m_layout;
    }

private:
    ColumnLayout m_layout;
    DBTable *m_table;
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

    ConstKind kind() const {
        return m_const_kind;
    }

    S64 as_int() const {
        return reinterpret_cast<S64>(m_data);
    }

    bool as_bool() const {
        return static_cast<bool>(reinterpret_cast<U64>(m_data));
    }

    ok::String *as_string() {
        return static_cast<ok::String *>(m_data);
    }

private:
    explicit ConstDBValue(SQL::Type type, ConstKind kind, void *data) : DBValue{type, Kind::CONSTANT}, m_const_kind{kind}, m_data{data} {}

    ConstKind m_const_kind;
    void *m_data;
};

class CompareDBValue : public DBValue {
public:
    CompareDBValue(DBValue *lhs, DBValue *rhs) : DBValue{lhs->type(), Kind::COMPARE}, m_lhs{lhs}, m_rhs{rhs} {
        OK_ASSERT(lhs->is_compatible_with(rhs));
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

class NoneDBValue : public DBValue {
public:
    NoneDBValue() : DBValue{SQL::TYPE_NULL, Kind::NONE} {}
};

class PairDBValue : public DBValue {
public:
    PairDBValue() = delete;

    DBValue *lhs() {
        return m_lhs;
    }

    DBValue *rhs() {
        return m_rhs;
    }

protected:
    PairDBValue(SQL::Type type,
                Kind kind,
                DBValue *lhs,
                DBValue *rhs) : DBValue{type, kind},
                                m_lhs{lhs},
                                m_rhs{rhs} {
    }

    DBValue *m_lhs;
    DBValue *m_rhs;
};

class ConcatDBValue : public PairDBValue {
public:
    ConcatDBValue(DBValue *lhs, DBValue *rhs) : PairDBValue{lhs->type(), Kind::CONCAT, lhs, rhs} {
        OK_ASSERT(lhs->is_compatible_with(rhs));
    }
};

class ImageDataDBValue : public DBValue {
public:
    ImageDataDBValue(U64 width, U64 height, ok::Slice<U8> data, PixelFormat format) : DBValue{SQL::TYPE_IMAGE, Kind::IMAGE_DATA}, m_width{width}, m_height{height}, m_data{data}, m_format{format} {}

    U64 width() {
        return m_width;
    }

    U64 height() {
        return m_height;
    }

    ok::Slice<U8> data() {
        return m_data;
    }

    PixelFormat format() {
        return m_format;
    }

private:
    U64 m_width;
    U64 m_height;
    ok::Slice<U8> m_data;
    PixelFormat m_format;
};
} // namespace xmdb
