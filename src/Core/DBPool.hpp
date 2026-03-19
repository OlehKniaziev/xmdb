#pragma once

#include "DBValue.hpp"
#include "DBTable.hpp"
#include "ok.hpp"

#include "QueryExecutionContext.hpp"

namespace xmdb {
/**
 * @brief Manages a pool of databases and execution contexts.
 */
struct DBPool {
    /**
     * @brief Constructs a new DBPool.
     * @param allocator The allocator to use for pool resources.
     */
    DBPool(ok::Allocator *allocator);

    /**
     * @brief Retrieves a database descriptor by name.
     * @param name The name of the database.
     * @return A pointer to the database descriptor, or nullptr if not found.
     */
    DBDescriptor *get_db(StringView name);

    /**
     * @brief Creates a new database with the given name.
     * @param name The name of the new database.
     * @return A pointer to the newly created database descriptor.
     */
    DBDescriptor *create_db(StringView name);

    /**
     * @brief Rents an empty execution context for a specific database and user.
     * @param db The database to use for execution.
     * @param user The user associated with the execution.
     * @return A pointer to the rented execution context.
     */
    QueryExecutionContext *rent_empty_execution_context(DBDescriptor *db, DBUser *user);

    /**
     * @brief Returns an execution context to the pool.
     * @param context The execution context to return.
     */
    void return_execution_context(QueryExecutionContext *context);

    ok::Allocator *allocator;               ///< The allocator for the pool.
    QueryExecutionContext *execution_contexts; ///< List of available execution contexts.
    DBDescriptor *db_descriptors;           ///< List of database descriptors.
};
} // namespace xmdb
