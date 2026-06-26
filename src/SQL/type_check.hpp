#ifndef XMDB_SQL_TYPE_CHECK_HPP
#define XMDB_SQL_TYPE_CHECK_HPP

#include <Core/SourceLocation.hpp>
#include <Core/util.hpp>
#include "ir.hpp"

namespace xmdb::SQL {
/**
 * @brief Result of compiling and type-checking a query.
 */
struct TypedCompiledQuery {
    CompiledQuery untyped;               ///< The underlying untyped compiled query.
    Table<U32, TypedTableSchema> table_types; ///< Mapping of instruction IDs to their resulting table schemas.
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
