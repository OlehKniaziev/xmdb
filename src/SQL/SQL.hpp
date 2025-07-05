#pragma once

#include "type_check.hpp"

namespace xmdb::SQL {
bool compile_and_type_check_source(ok::ArenaAllocator *arena, StringView source, TypedCompiledQuery *typed_query, String *error);
} // namespace xmdb::SQL
