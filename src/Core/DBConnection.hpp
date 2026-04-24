#pragma once

#include <SQL/ir.hpp>

#include "DBPool.hpp"
#include "ok.hpp"

namespace xmdb
{
/**
 * @brief Stores the results of a database query.
 */
struct QueryResults
{
    Optional<ErrorWithSourceLocation>
            error; ///< Error information if the query failed.
    Optional<DBTable *> value; ///< The resulting table if the query succeeded
                               ///< and had any output.
};

/**
 * @brief Represents an active connection to a database.
 */
struct DBConnection
{
    /**
     * @brief Constructs a new DBConnection.
     * @param pool The database pool this connection belongs to.
     * @param db The database descriptor for this connection.
     * @param user The user associated with this connection.
     */
    explicit DBConnection(DBPool *pool, DBDescriptor *db, DBUser *user);

    /**
     * @brief Executes a compiled SQL query.
     * @param query The compiled query to execute.
     * @param results Pointer to a structure where results will be stored.
     */
    void execute(SQL::TypedCompiledQuery *query, QueryResults *results);

    /**
     * @brief Closes the connection and releases resources.
     */
    void close();

    DBPool *db_pool; ///< Pointer to the database pool.
    DBUser *user; ///< Pointer to the associated user.
    DBDescriptor *db; ///< Pointer to the database descriptor.
    SQL::IrContext ir_ctx; ///< The intermediate representation context for
                           ///< execution of queries.
};
} // namespace xmdb
