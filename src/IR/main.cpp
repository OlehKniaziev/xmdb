#include <SQL/ir.hpp>
#include <SQL/SQL.hpp>
#include <SQL/Parser.hpp>
#include <SQL/util.hpp>
#include <Core/util.hpp>

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
    if (argc == 1) {
        h(1, "not enough arguments");
    }

    --argc;
    ++argv;

    bool untyped = false;
    bool parse_tables = false;
    int flags = 0;

    ok::ArenaAllocator arena{};

    ok::List<ok::StringView> tables = ok::List<ok::StringView>::alloc(&arena);

    for (int i = 0; i < argc - 1; ++i) {
        if (parse_tables) {
            const char *tables_str = argv[i];
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
            ++flags;
            parse_tables = false;
            continue;
        }

        if ((strcmp(argv[i], "-u") == 0) || (strcmp(argv[i], "--untyped") == 0)) {
            ++flags;
            untyped = true;
        } else if ((strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--tables") == 0)) {
            ++flags;
            parse_tables = true;
        } else {
            ok::String msg = ok::String::format(ok::temp_allocator(), "unrecognized flag %s", argv[i]);
            h(1, msg.cstr());
        }
    }

    argc -= flags;
    argv += flags;

    if (argc != 1) {
        h(1, "only 1 argument expected");
    }

    const char *src = argv[argc - 1];

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
