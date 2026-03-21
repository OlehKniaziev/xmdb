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
    using Flags = U16;
    enum : Flags {
        F_EPHEMERAL = 1 << 0, ///< No catalog data will be loaded or stored.
    };

    /**
     * @brief Constructs a new DBPool.
     * @param allocator The allocator to use for pool resources.
     * @param flags Flags for this pool.
     */
    DBPool(ok::Allocator *allocator, Flags flags = 0);

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

    /**
     * @brief Tries to sync this pool's catalog to disk.
     * @return The number of bytes written on success, error string otherwise.
     */
    Result<UZ, ok::String> sync_to_disk();

    /**
     * @brief Returns whether this DBPool is ephemeral.
     * @return true if the F_EPHEMERAL flag is set, false otherwise
     */
    bool ephemeral() const {
        return (flags & F_EPHEMERAL) == 1;
    }

    ok::Allocator *allocator;               ///< The allocator for the pool.
    Flags flags;                            ///< Behavior flags.
    QueryExecutionContext *execution_contexts; ///< List of available execution contexts.
    DBDescriptor *db_descriptors;           ///< List of database descriptors.
    ok::StringView catalog_file_name;        ///< File name for the pool's catalog.
};
} // namespace xmdb
