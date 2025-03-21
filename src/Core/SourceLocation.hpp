#ifndef XMDB_SOURCELOCATION_HPP
#define XMDB_SOURCELOCATION_HPP

#include <cstdint>
#include "ok.hpp"

namespace xmdb {
struct SourceLocation {
    uint32_t line;
    uint32_t column;
    uint32_t length;

    inline bool operator==(const SourceLocation& other) const {
        return line == other.line && column == other.column && length == other.length;
    }
};
}; // namespace xmdb

#endif // XMDB_SOURCELOCATION_HPP
