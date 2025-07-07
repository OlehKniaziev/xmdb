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
        TableStream<bool> boolean;
        TableStream<S64> integer;
        TableStream<String> string;
    } u;

    static DBValue boolean(TableStream<bool> b) {
        DBValue value{};
        value.type = SQL::Type::TYPE_BOOL;
        value.u.boolean = b;
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

    static DBValue concat(ok::Allocator *allocator, DBValue lhs, DBValue rhs) {
        if (lhs.type != rhs.type) {
            OK_PANIC("Type mismatch for concatenation of DBValues");
        }

        switch (lhs.type) {
        case SQL::TYPE_INT: {
            TableStream<S64> lhs_stream = lhs.u.integer;
            TableStream<S64> rhs_stream = rhs.u.integer;
            TableStream<S64> result_stream = TableStream<S64>::concat(allocator, lhs_stream, rhs_stream);
            return DBValue::integer(result_stream);
        }
        case SQL::TYPE_STRING: {
            TableStream<String> lhs_stream = lhs.u.string;
            TableStream<String> rhs_stream = rhs.u.string;
            TableStream<String> result_stream = TableStream<String>::concat(allocator, lhs_stream, rhs_stream);
            return DBValue::string(result_stream);
        }
        case SQL::TYPE_BOOL: {
            TableStream<bool> lhs_stream = lhs.u.boolean;
            TableStream<bool> rhs_stream = rhs.u.boolean;
            TableStream<bool> result_stream = TableStream<bool>::concat(allocator, lhs_stream, rhs_stream);
            return DBValue::boolean(result_stream);
        }
        case SQL::TYPE_FLOAT:
        case SQL::TYPE_DOUBLE:
        case SQL::TYPE_IMAGE: OK_TODO();
        case SQL::TYPE_TABLE: OK_PANIC("Cannot concatenate table values");
        case SQL::TYPE_MAX:
        case SQL::TYPE_NULL: OK_UNREACHABLE();
        }

        OK_UNREACHABLE();
    }

    DBValue cmp(ok::Allocator *, DBValue);
};
} // namespace xmdb
