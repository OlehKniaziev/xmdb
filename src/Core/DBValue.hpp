#pragma once

#include <SQL/type_check.hpp>

#include "TableStream.hpp"

namespace xmdb {
struct DBTable;

struct DBValue {
    DBValue() : u{0} {
    }

    SQL::Type type;
    union {
        DBTable *table;
        TableStream<bool> b;
        TableStream<S64> integer;
        TableStream<String> string;
    } u;

    static DBValue boolean(TableStream<bool> b) {
        DBValue value{};
        value.type = SQL::Type::TYPE_BOOL;
        value.u.b = b;
        return value;
    }

    static DBValue integer(TableStream<S64> integer) {
        DBValue value{};
        value.type = SQL::Type::TYPE_INT;
        value.u.integer = integer;
        return value;
    }

    static DBValue table(DBTable *table) {
        DBValue value{};
        value.type = SQL::Type::TYPE_TABLE;
        value.u.table = table;
        return value;
    }

    static DBValue string(TableStream<String> string) {
        DBValue value{};
        value.type = SQL::Type::TYPE_STRING;
        value.u.string = string;
        return value;
    }

    DBValue cmp(ok::Allocator *, DBValue);
};
} // namespace xmdb
