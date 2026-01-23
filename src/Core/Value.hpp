#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"
#include "FixedString.hpp"

namespace xmdb {
class Value {
public:
    Value() = delete;

    static Value integer(S64 value) {
        return Value{SQL::TYPE_INT, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    static Value boolean(bool value) {
        return Value{SQL::TYPE_BOOL, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    static Value string(ok::Allocator *a, FixedString s) {
        FixedString *ptr = a->alloc<FixedString>();
        memcpy(ptr, reinterpret_cast<U8 *>(&s), sizeof(s));
        return Value{SQL::TYPE_STRING, reinterpret_cast<void *>(ptr)};
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

    S64 as_int() {
        OK_VERIFY(type() == SQL::TYPE_INT);
        return static_cast<S64>(reinterpret_cast<U64>(m_data));
    }

    bool as_bool() {
        OK_VERIFY(type() == SQL::TYPE_BOOL);
        return static_cast<bool>(reinterpret_cast<U64>(m_data));
    }

    FixedString as_string() {
        OK_VERIFY(type() == SQL::TYPE_STRING);
        return *reinterpret_cast<FixedString *>(m_data);
    }

    SQL::Type type() const {
        return m_type;
    }

    Value compare(Value);

private:
    Value(SQL::Type type, void *data) : m_type{type}, m_data{data} {}

    SQL::Type m_type;
    void *m_data;
};
} // namespace xmdb
