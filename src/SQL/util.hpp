#pragma once

#include "Lexer.hpp"

namespace xmdb::SQL {
/**
 * @brief Formats an error message with its source location.
 * @param allocator The allocator to use for the resulting string.
 * @param location The source location of the error.
 * @param message The error message.
 * @return The formatted error string.
 */
static inline String format_error(ok::Allocator *allocator, SourceLocation location, ok::StringView message) {
    return String::format(allocator, "%d:%d: " OK_SV_FMT, location.line, location.column, OK_SV_ARG(message));
}
} // namespace xmdb::SQL
