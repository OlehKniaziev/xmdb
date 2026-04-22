#pragma once

#include "DBTable.hpp"
#include "DBUser.hpp"

namespace xmdb
{
/**
 * @brief Describes a database and manages its tables and users.
 */
struct DBDescriptor
{
    /**
     * @brief Allocates a new DBDescriptor.
     * @param allocator The allocator to use for the descriptor.
     * @param name The name of the database.
     * @return A pointer to the newly allocated DBDescriptor.
     */
    static DBDescriptor *alloc(ok::Allocator *allocator, ok::StringView name);

    /**
     * @brief Adds a user to the database.
     * @param user The user to add.
     */
    inline void add_user(DBUser *user)
    {
        user->next = users;
        users = user;
    }

    /**
     * @brief Finds a user by name.
     * @param name The name of the user to find.
     * @return An optional containing the user if found, or empty otherwise.
     */
    inline Optional<DBUser *> find_user(StringView name)
    {
        for (DBUser *user = users; user != nullptr; user = user->next)
        {
            if (user->name == name)
            {
                return user;
            }
        }

        return {};
    }

    /**
     * @brief Creates a new table in the database.
     * @param allocator The allocator to use for the table.
     * @param name The name of the table.
     * @param flags The table flags (e.g., storage type).
     * @param column_count The number of columns in the table.
     * @param column_names The names of the columns.
     * @param column_types The types of the columns.
     * @return A pointer to the newly created table.
     */
    DBTable *create_new_table(ok::Allocator *allocator, ok::StringView name,
                              DBTable::Flags flags, UZ column_count,
                              ok::Slice<ok::StringView> column_names,
                              ok::Slice<SQL::ColumnType> column_types);

    /**
     * @brief Loads an existing table from disk.
     * @param allocator The allocator to use for the table.
     * @param name The name of the table.
     * @param flags The table flags (e.g., storage type).
     * @param column_count The number of columns in the table.
     * @param column_names The names of the columns.
     * @param column_types The types of the columns.
     * @return A result with an allocated table on success, an error string
     * otherwise.
     */
    Result<DBTable *, ok::String> load_existing_table(
            ok::Allocator *allocator, ok::StringView name, DBTable::Flags flags,
            UZ column_count, ok::Slice<ok::StringView> column_names,
            ok::Slice<SQL::ColumnType> column_types);

    DBDescriptor *next; ///< Pointer to the next database descriptor in a list.
    ok::StringView name; ///< The name of the database.
    DBUser *users; ///< Pointer to the head of the users list.
    ok::List<DBTable *> tables; ///< List of tables in the database.
};
} // namespace xmdb
