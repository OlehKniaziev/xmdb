#include <gtest/gtest.h>
#include <SQL/Parser.hpp>
#include <SQL/type_check.hpp>

using namespace ok::literals;
using namespace xmdb::SQL;

TEST(type_checking, basic) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    SELECT column1, column2 FROM MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext typing_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &typing_ctx));
}
