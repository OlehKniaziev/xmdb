#pragma once

#include "Lexer.hpp"

namespace xmdb::SQL {
static inline String format_error(ok::Allocator *allocator, SourceLocation location, ok::StringView message) {
    return String::format(allocator, "%d:%d: " OK_SV_FMT, location.line, location.column, OK_SV_ARG(message));
}
} // namespace xmdb::SQL
