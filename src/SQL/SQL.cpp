#include "SQL.hpp"

#include "Parser.hpp"
#include "type_check.hpp"
#include "util.hpp"

namespace xmdb::SQL {
bool compile_and_type_check_source(ok::ArenaAllocator *arena, StringView source, CompiledQuery *compiled_query, String *error) {
    Parser parser{arena, source};

    Optional<Query> query = parser.query();
    if (!query.has_value()) {
        Parser::Error parser_error = parser.error.get();
        StringView error_message = parser_error.message.view();
        *error = format_error(arena, parser_error.location, error_message);
        return false;
    }

    IrContext ir_ctx{arena, source};

    if (!ir_compile_query(&query.value, &ir_ctx, compiled_query)) {
        // TODO: Report error.
        return false;
    }

    TypingContext t_ctx{arena, source};
    if (!type_check_query(compiled_query, &t_ctx)) {
        // TODO: Report error.
        return false;
    }

    return true;
}
} // namespace xmdb::SQL
