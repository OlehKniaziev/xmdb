#include <gtest/gtest.h>
#include <SQL/Parser.hpp>
#include <SQL/ir.hpp>
#include <SQL/type_check.hpp>

using namespace ok::literals;
using namespace xmdb::SQL;

TEST(ir, basic) {
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

    ASSERT_EQ(ir_ctx.database_schemas.count, 1);
    ASSERT_EQ(ir_ctx.database_schemas[0].name, "default"_sv);

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, select) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text,
        column3 text
    );
    SELECT column1, column2 = column3, column3 FROM MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, create_database) {
    ok::ArenaAllocator arena{};
    auto source = "CREATE DATABASE DB; USE DB;"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, create_table) {
    ok::ArenaAllocator arena{};
    auto source = "CREATE TABLE Table (id int); SELECT id FROM Table;"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, drop_database) {
    ok::ArenaAllocator arena{};
    auto source = "DROP DATABASE default; CREATE DATABASE DB; DROP DATABASE DB;"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, drop_table) {
    ok::ArenaAllocator arena{};
    auto source = "CREATE TABLE Table (id int); DROP TABLE Table;"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, insert) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    INSERT INTO MyTable (column1) VALUES (1), (2), (3);)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, update) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    UPDATE MyTable SET column1 = 1, column2 = "hello";)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}

TEST(ir, delete_) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    DELETE FROM MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    IrContext ir_ctx{&arena, source};

    ASSERT_TRUE(ir_compile_query(&query.value, &ir_ctx));

    TypingContext t_ctx = new_typing_context(&arena, source);
    ASSERT_TRUE(type_check_ir(&ir_ctx.ir_emitter, &t_ctx));
}
