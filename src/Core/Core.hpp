#pragma once

#include "DBConnection.hpp"

namespace xmdb {
bool compile_and_execute_source(ok::ArenaAllocator *arena,
                                DBConnection *connection,
                                StringView source,
                                QueryResults *results,
                                String *error);
} // namespace xmdb
