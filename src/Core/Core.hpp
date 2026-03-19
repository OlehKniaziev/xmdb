#pragma once

#include "DBConnection.hpp"

namespace xmdb {
/**
 * @brief Compiles and executes SQL source code.
 *
 * @param arena The allocator for temporary data during compilation and execution.
 * @param connection The database connection to execute the query on.
 * @param source The SQL source string to execute.
 * @param results Pointer to a structure to store the query results.
 * @param error Pointer to a string to store error messages if execution fails.
 * @return true if compilation and execution succeeded, false otherwise.
 */
bool compile_and_execute_source(ok::ArenaAllocator *arena,
                                DBConnection *connection,
                                StringView source,
                                QueryResults *results,
                                String *error);
} // namespace xmdb
