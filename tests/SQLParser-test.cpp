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

TEST(SQLParser, use_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "USE some_database;"_sv;
    SQLParser parser{&arena, source};

    auto use_stmt = parser.use_stmt();
    EXPECT_TRUE(use_stmt.has_value());

    EXPECT_EQ(use_stmt.value->database, "some_database"_sv);
}

TEST(SQLParser, insert_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "INSERT INTO Users (name, age) VALUES (oleh, some_age);"_sv;
    SQLParser parser{&arena, source};

    auto insert_stmt = parser.insert_stmt();
    EXPECT_TRUE(insert_stmt.has_value());

    EXPECT_EQ(insert_stmt.value->table->type, SQLExpr::IDENT);
    EXPECT_EQ(static_cast<SQLExprIdentifier*>(insert_stmt.value->table)->value, "Users"_sv);

    EXPECT_EQ(insert_stmt.value->columns.count, 2);
    EXPECT_EQ(insert_stmt.value->columns[0], "name"_sv);
    EXPECT_EQ(insert_stmt.value->columns[1], "age"_sv);

    EXPECT_EQ(insert_stmt.value->values.count, 2);

    auto values = insert_stmt.value->values.cast<SQLExprIdentifier*>();
    EXPECT_EQ(values[0]->value, "oleh"_sv);
    EXPECT_EQ(values[1]->value, "some_age"_sv);

    EXPECT_EQ(insert_stmt.value->values_counts.count, 1);
    EXPECT_EQ(insert_stmt.value->values_counts[0], 2);
}

TEST(SQLParser, eof_error) {
    ok::ArenaAllocator arena{};
    auto source = ""_sv;
    SQLParser parser{&arena, source};

    EXPECT_TRUE(parser.is_eof());

    auto expr = parser.expression();
    EXPECT_FALSE(expr.has_value());

    EXPECT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    EXPECT_EQ(error.message, "unexpected EOF"_sv);

    auto expected_location = SourceLocation{1, 1, 0};
    EXPECT_EQ(error.location, expected_location);
}

TEST(SQLParser, token_mismatch_error) {
    ok::ArenaAllocator arena{};
    auto source = "not_select error FROM parser_errors"_sv;
    SQLParser parser{&arena, source};

    auto opt_stmt = parser.select_stmt();
    EXPECT_FALSE(opt_stmt.has_value());
    EXPECT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    EXPECT_EQ(error.message, "expected a token of type SELECT, but got identifier instead"_sv);

    auto expected_location = SourceLocation{1, 1, (uint32_t)strlen("not_select")};
    EXPECT_EQ(error.location, expected_location);
}