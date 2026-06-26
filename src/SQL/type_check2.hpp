#pragma once

#include <Core/Result.hpp>
#include <Core/util.hpp>
#include "ast.hpp"

using ok::Allocator;
using ok::List;
using ok::Optional;
using ok::Slice;
using ok::String;
using ok::StringView;
using ok::Table;

namespace xmdb::SQL {
#define XMDB_ENUM_COLUMN_TYPES \
    X(INTEGER) \
    X(FLOAT) \
    X(DOUBLE) \
    X(TEXT) \
    X(BOOLEAN) \
    X(PNG)

/**
 * @brief Supported types for table columns.
 */
enum class ColumnType {
#define X(type) type,
XMDB_ENUM_COLUMN_TYPES
#undef X
};

/**
 * @brief Converts a ColumnType to its string representation.
 * @param type The column type.
 * @return The name of the column type.
 */
const char *column_type_to_string(ColumnType type);

/**
 * @brief Parses a column type from a string view.
 * @param sv The string view to parse.
 * @return The parsed ColumnType, or empty if unknown.
 */
ok::Optional<ColumnType> parse_column_type(ok::StringView sv);

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
    Slice<String> column_names; ///< Names of the columns.
    Slice<Type> column_types;   ///< Types of the columns.
};

/**
 * @brief Signature of a SQL function.
 */
struct FunctionSignature {
    Type return_type;            ///< The type of the value returned by the function.
    Slice<Type> parameter_types; ///< The types of the function parameters.
};

struct TypedQuery { /* TODO */ };

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

    /**
     * @brief Type checks a compiled query.
     * @param query The query to check.
     * @param out Pointer to store the resulting TypedCompiledQuery.
     * @return true if successful, false otherwise.
     */
    Result<TypedQuery, ErrorWithSourceLocation> type_check_query(Query *query);
};
};
