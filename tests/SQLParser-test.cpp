#include <gtest/gtest.h>
#include "SQLParser.hpp"

using namespace xmdb;
using namespace ok::literals;

TEST(SQLParser, expression) {
    ok::ArenaAllocator arena{};
    auto source = "some_very_nice_expression"_sv;
    SQLParser parser{&arena, source};

    auto ident = parser.expression();
    EXPECT_TRUE(ident.has_value());
    EXPECT_EQ(ident.value->type, SQLExpr::IDENT);
    EXPECT_EQ(static_cast<SQLExprIdentifier*>(ident.value)->value, "some_very_nice_expression"_sv);
}

TEST(SQLParser, select_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "SELECT id, name FROM test;"_sv;
    SQLParser parser{&arena, source};

    auto select = parser.select_stmt();
    EXPECT_TRUE(select.has_value());

    auto select_stmt = select.value;

    EXPECT_EQ(select_stmt->exprs.count, 2);
    EXPECT_EQ(select_stmt->exprs[0]->type, SQLExpr::IDENT);
    EXPECT_EQ(select_stmt->exprs[1]->type, SQLExpr::IDENT);

    EXPECT_EQ(select_stmt->table->type, SQLExpr::IDENT);
}