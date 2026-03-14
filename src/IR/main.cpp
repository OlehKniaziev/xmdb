#include <SQL/ir.hpp>
#include <SQL/SQL.hpp>
#include <SQL/Parser.hpp>
#include <SQL/util.hpp>
#include <Core/util.hpp>
#include <ArgParser/ArgParser.hpp>

const char *help_message =
    "ir SOURCE - Usage\n\n"
    "\tSOURCE - xmdb SQL source.\n"
;

void h(int code, const char *msg = nullptr) {
    printf("%s\n", help_message);
    if (msg != nullptr) {
        printf("ERROR: %s\n", msg);
    }
    exit(code);
}

void err(const char *why, xmdb::ErrorWithSourceLocation error) {
    ok::String message = xmdb::SQL::format_error(ok::temp_allocator(), error.location, error.message.view());
    printf("%s: %s\n", why, message.cstr());
    exit(1);
}

int main(int argc, char **argv) {
    xmdb::argparser::ArgParser arg_parser{argc, argv};

    bool untyped = false;
    const char *tables_str;

    arg_parser.boolean("untyped", &untyped, "Whether to type check the compiled query");
    arg_parser.string("tables", &tables_str, "List of comma-separated tables to define for the semantic analysis. Useful with the '-untyped' flag", "");

    if (!arg_parser.parse()) {
        ok::StringView error_message = arg_parser.error_message();
        arg_parser.help();
        xmdb::dief("\nFailed to parse command line arguments: " OK_SV_FMT, OK_SV_ARG(error_message));
    }

    if (arg_parser.positionals().count == 0) {
        arg_parser.help();
        xmdb::dief("\nFailed to parse command line arguments: expected at least one positional argument");
    }

    const char *src = arg_parser.positionals()[0];

    ok::ArenaAllocator arena{};

    ok::List<ok::StringView> tables = ok::List<ok::StringView>::alloc(&arena);

    while (*tables_str) {
        const char *comma = strchr(tables_str, ',');
        UZ len = 0;
        if (comma == NULL) {
            len = strlen(tables_str);
        } else {
            len = (UZ)(comma - tables_str);
        }
        tables.push(ok::StringView{tables_str, len});
        if (comma == NULL) break;
        tables_str = comma + 1;
    }

    ok::StringView source{src};

    xmdb::SQL::Parser parser{&arena, source};

    ok::Optional<xmdb::SQL::Query> q_opt = parser.query();
    if (!q_opt) {
        err("Failed to parse the input", parser.error.get());
    }

    xmdb::SQL::Query q = q_opt.get();
    xmdb::SQL::IrContext ir_ctx{&arena, source};
    xmdb::SQL::CompiledQuery compiled_query{};

    for (UZ table_idx = 0; table_idx < tables.count; ++table_idx) {
        ok::StringView table_name = tables[table_idx];
        ir_ctx.alloc_table_schema(ir_ctx.active_db_id, table_name.to_string(&arena), false);
    }

    bool result = xmdb::SQL::ir_compile_query(&q,
                                              &ir_ctx,
                                              &compiled_query);
    if (!result) {
        err("Failed to compile the input", ir_ctx.error.get());
    }

    xmdb::SQL::TypingContext t_ctx{&arena, source};

    xmdb::SQL::TypedCompiledQuery typed_query{};

    if (untyped) goto good;

    result = xmdb::SQL::type_check_query(&compiled_query, &t_ctx, &typed_query);
    if (!result) {
        err("Failed to type check the input", t_ctx.error.get());
    }

good:

    ok::String ir_string = stringify_ir(&arena, &compiled_query);
    printf("%s\n", ir_string.cstr());
    return 0;
}
