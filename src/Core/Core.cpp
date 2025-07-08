#include "Core.hpp"

#include <SQL/SQL.hpp>

namespace xmdb {
bool compile_and_execute_source(ok::ArenaAllocator *arena,
                                DBConnection *connection,
                                StringView source,
                                QueryResults *results,
                                String *error) {
    SQL::TypedCompiledQuery typed_query{};
    connection->ir_ctx.source = source;
    if (!compile_and_type_check_source(arena,
                                       source,
                                       &typed_query,
                                       error,
                                       &connection->ir_ctx)) {
        return false;
    }

    connection->execute(&typed_query, results);
    return results->ok;
}
} // namespace xmdb
