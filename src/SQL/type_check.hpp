#ifndef XMDB_SQL_TYPE_CHECK_HPP
#define XMDB_SQL_TYPE_CHECK_HPP

#include <Core/SourceLocation.hpp>
#include "ir.hpp"

namespace xmdb::SQL {
enum Type {
    TYPE_INT,
    TYPE_STRING,
    TYPE_IMAGE,
    TYPE_BOOL,
    TYPE_NULL,
    TYPE_TABLE,
    TYPE_FLOAT,
    TYPE_DOUBLE,

    TYPE_MAX,
};

Type column_type_to_type(ColumnType);

struct TypedTableSchema {
    Slice<Optional<String>> column_names;
    Slice<Type> column_types;
};

struct TypingContextError {
    SourceLocation location;
    String message;
};

struct TypedCompiledQuery {
    CompiledQuery untyped;
    Table<U32, TypedTableSchema> table_types;
};

struct TypingContext {
    TypingContext(Allocator *allocator, StringView source);

    Allocator *allocator;
    Table<U32, Type> ir_instruction_types;
    Table<U32, TypedTableSchema> table_types;
    List<U32> emitted_columns;
    StringView source; // used only for error reporting
    Optional<TypingContextError> error{};
};

bool type_check_query(CompiledQuery *query, TypingContext *ctx, TypedCompiledQuery *out);
};

#endif // XMDB_SQL_TYPE_CHECK_HPP
