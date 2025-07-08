#include "SQL.hpp"

#include "Parser.hpp"
#include "type_check.hpp"
#include "util.hpp"

namespace xmdb::SQL {
bool compile_and_type_check_source(ok::ArenaAllocator *arena,
                                   StringView source,
                                   TypedCompiledQuery *typed_query,
                                   String *error,
                                   IrContext *ir_ctx) {
    Parser parser{arena, source};

    Optional<Query> query = parser.query();
    if (!query.has_value()) {
        Parser::Error parser_error = parser.error.get();
        StringView error_message = parser_error.message.view();
        *error = format_error(arena, parser_error.location, error_message);
        return false;
    }

    if (ir_ctx == nullptr) {
        *ir_ctx = IrContext{arena, source};
    }

    if (!ir_compile_query(&query.value, ir_ctx, &typed_query->untyped)) {
        IrContext::Error ir_error = ir_ctx->error.get();
        StringView error_message = ir_error.message.view();
        *error = format_error(arena, ir_error.location, error_message);
        return false;
    }

    TypingContext t_ctx{arena, source};
    if (!type_check_query(&typed_query->untyped, &t_ctx, typed_query)) {
        TypingContext::Error typing_error = t_ctx.error.get();
        StringView error_message = typing_error.message.view();
        *error = format_error(arena, typing_error.location, error_message);
        return false;
    }

    return true;
}
} // namespace xmdb::SQL
