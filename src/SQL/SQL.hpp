#pragma once

#include "ir.hpp"

namespace xmdb::SQL {
bool compile_and_type_check_source(ok::ArenaAllocator *arena, StringView source, CompiledQuery *compiled_query, String *error);
} // namespace xmdb::SQL
