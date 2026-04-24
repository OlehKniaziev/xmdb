#pragma once

#include "ok.hpp"

namespace xmdb
{
/**
 * @brief Global storage for built-in functions and other static data.
 */
struct StaticStorage
{
    ok::Table<ok::StringView, void *>
            builtin_functions; ///< Table mapping function names to their
                               ///< implementations.
};

/**
 * @brief Retrieves the global StaticStorage instance, creating it if it doesn't
 * exist.
 * @return Pointer to the StaticStorage instance.
 */
StaticStorage *make_or_get_static_storage();
} // namespace xmdb
