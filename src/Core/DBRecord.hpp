#pragma once

#include "ok.hpp"
#include "Value.hpp"

namespace xmdb {
/**
 * @brief Represents a single record (row) in a database table.
 */
struct DBRecord {
    U8 *buffer;           ///< The buffer containing the record data.
    UZ buffer_size;       ///< The size of the buffer.
    U64 primary_key_value; ///< The value of the primary key for this record.
};

/**
 * @brief Creates a new DBRecord with a buffer sized according to the table layout.
 * @param allocator The allocator to use for the buffer.
 * @param layout The layout of the table.
 * @return The newly created DBRecord.
 */
DBRecord db_record_create(ok::Allocator *allocator, TableLayout layout);
} // namespace xmdb
