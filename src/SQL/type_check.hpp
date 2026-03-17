#ifndef XMDB_SQL_TYPE_CHECK_HPP
#define XMDB_SQL_TYPE_CHECK_HPP

#include <Core/SourceLocation.hpp>
#include <Core/util.hpp>
#include "ir.hpp"

namespace xmdb::SQL {
enum Type {
    TYPE_INT,
    TYPE_STRING,
    TYPE_PNG,
    TYPE_IMAGE_CHUNK,
    TYPE_BOOL,
    TYPE_NULL,
    TYPE_TABLE,
    TYPE_FLOAT,
    TYPE_DOUBLE,
};

Type column_type_to_type(ColumnType);
bool is_image(Type);

const char *type_name(Type);

struct TypedTableSchema {
    Slice<Optional<String>> column_names;
    Slice<Type> column_types;
};

struct TypedCompiledQuery {
    CompiledQuery untyped;
    Table<U32, TypedTableSchema> table_types;
};

struct FunctionSignature {
    Type return_type;
    Slice<Type> parameter_types;
};

struct TypingContext {
    TypingContext(Allocator *allocator, StringView source);

    Allocator *allocator;
    Table<U32, Type> ir_instruction_types;
    Table<U32, TypedTableSchema> table_types;
    Table<StringView, FunctionSignature> function_signatures;
    List<U32> emitted_columns;
    List<U32> call_args;
    List<StringView> insert_column_names;
    List<U32> insert_column_values;
    StringView source; // NOTE(oleh): Used only for error reporting.
    Optional<ErrorWithSourceLocation> error{};
};

bool type_check_query(CompiledQuery *query, TypingContext *ctx, TypedCompiledQuery *out);
};

#endif // XMDB_SQL_TYPE_CHECK_HPP
