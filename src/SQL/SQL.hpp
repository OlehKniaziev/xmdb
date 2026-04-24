/// @file SQL.hpp
/// @brief SQL convenience routines.
#pragma once

#include "type_check.hpp"

namespace xmdb::SQL
{
/// @defgroup SQL_API SQL API
/// @{

/// Compiles and type checks the provided SQL source.
///
/// @param[in]  arena       Arena used to allocate everything.
/// @param      source      The SQL source.
/// @param[out] typed_query Output typed query pointer.
/// @param[out] error       Pointer to the error string.
/// @param[in]  ir_ctx      Optional IR context used for semantic analysis.
///                         If nullptr is provided, constructs a default IR
///                         context.
///
/// @return true if successful, false otherwise. The typed_query parameter is
/// written to only on success;
///         the error parameter is written to only on failure.
bool compile_and_type_check_source(ok::ArenaAllocator *arena, StringView source,
                                   TypedCompiledQuery *typed_query,
                                   String *error, IrContext *ir_ctx = nullptr);

/// @}
} // namespace xmdb::SQL
