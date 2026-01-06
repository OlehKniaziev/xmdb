#include <SQL/Parser.hpp>
#include <SQL/ir.hpp>

using namespace ok::literals;
using namespace xmdb::SQL;

int main() {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    SELECT column1, column2 FROM MyTable;
    INSERT INTO MyTable(column1, column2) VALUES (1, "1"), (2, "2"), (3, "3");
    UPDATE MyTable SET column1 = 1337;
    DELETE FROM MyTable;
    DROP TABLE MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    OK_ASSERT(query.has_value());

    IrContext ir_ctx{&arena, source};

    CompiledQuery compiled_query{};

    OK_ASSERT(ir_compile_query(&query.value, &ir_ctx, &compiled_query));
    ok::println(stringify_ir(&arena, &compiled_query));
}
