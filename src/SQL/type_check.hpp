#ifndef XMDB_SQL_TYPE_CHECK_HPP
#define XMDB_SQL_TYPE_CHECK_HPP

#include <Core/SourceLocation.hpp>
#include <Core/util.hpp>
#include "ir.hpp"

namespace xmdb::SQL {
/**
 * @brief Supported SQL data types.
 */
enum Type {
    TYPE_INT,         ///< Integer.
    TYPE_STRING,      ///< String.
    TYPE_PNG,         ///< PNG image data.
    TYPE_IMAGE_CHUNK, ///< A chunk of an image.
    TYPE_BOOL,        ///< Boolean.
    TYPE_NULL,        ///< NULL value.
    TYPE_TABLE,       ///< A database table.
    TYPE_FLOAT,       ///< Single-precision floating point.
    TYPE_DOUBLE,      ///< Double-precision floating point.
};

/**
 * @brief Converts a ColumnType to a Type.
 * @param col_type The column type.
 * @return The corresponding data type.
 */
Type column_type_to_type(ColumnType col_type);

/**
 * @brief Checks if a type represents image data.
 * @param type The type to check.
 * @return true if it's an image type, false otherwise.
 */
bool is_image(Type type);

/**
 * @brief Gets the human-readable name of a type.
 * @param type The type.
 * @return The name string.
 */
const char *type_name(Type type);

/**
 * @brief Schema for a table with typed columns.
 */
struct TypedTableSchema {
    Slice<Optional<String>> column_names; ///< Names of the columns.
    Slice<Type> column_types;             ///< Types of the columns.
};

/**
 * @brief Result of compiling and type-checking a query.
 */
struct TypedCompiledQuery {
    CompiledQuery untyped;               ///< The underlying untyped compiled query.
    Table<U32, TypedTableSchema> table_types; ///< Mapping of instruction IDs to their resulting table schemas.
};

/**
 * @brief Signature of a SQL function.
 */
struct FunctionSignature {
    Type return_type;            ///< The type of the value returned by the function.
    Slice<Type> parameter_types; ///< The types of the function parameters.
};

/**
 * @brief Context for the type-checking phase.
 */
struct TypingContext {
    /**
     * @brief Constructs a new TypingContext.
     * @param allocator The allocator to use.
     * @param source The original SQL source.
     */
    TypingContext(Allocator *allocator, StringView source);

    Allocator *allocator;                             ///< The allocator.
    Table<U32, Type> ir_instruction_types;           ///< Mapping of instruction IDs to their result types.
    Table<U32, TypedTableSchema> table_types;        ///< Mapping of instruction IDs to table schemas.
    Table<StringView, FunctionSignature> function_signatures; ///< Mapping of function names to signatures.
    List<U32> emitted_columns;                        ///< Pending IDs of values emitted as columns of a query.
    List<U32> call_args;                              ///< IDs of values used as function arguments.
    List<StringView> insert_column_names;             ///< Pending column names for insertion.
    List<U32> insert_column_values;                   ///< Pending value instruction IDs for insertion.
    StringView source;                                ///< The original SQL source (for error reporting).
    Optional<ErrorWithSourceLocation> error;          ///< Error information if type checking failed.
};

/**
 * @brief Type checks a compiled query.
 * @param query The compiled query to check.
 * @param ctx The typing context.
 * @param out Pointer to store the resulting TypedCompiledQuery.
 * @return true if successful, false otherwise.
 */
bool type_check_query(CompiledQuery *query, TypingContext *ctx, TypedCompiledQuery *out);
};

#endif // XMDB_SQL_TYPE_CHECK_HPP
