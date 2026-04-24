#ifndef XMDB_SOURCELOCATION_HPP
#define XMDB_SOURCELOCATION_HPP

#include "ok.hpp"

namespace xmdb
{
/**
 * @brief Represents a location within a source file.
 */
struct SourceLocation
{
    uint32_t line; ///< The line number (1-based).
    uint32_t column; ///< The column number (1-based).
    uint32_t length; ///< The length of the source fragment.

    inline bool operator==(const SourceLocation &other) const
    {
        return line == other.line && column == other.column &&
               length == other.length;
    }
};
}; // namespace xmdb

#endif // XMDB_SOURCELOCATION_HPP
