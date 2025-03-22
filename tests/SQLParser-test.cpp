#include <gtest/gtest.h>
#include "SQL/Parser.hpp"

using namespace xmdb::SQL;
using namespace ok::literals;

TEST(SQLParser, expression) {
    ok::ArenaAllocator arena{};
    auto source = "some_very_nice_expression 999"_sv;
    Parser parser{&arena, source};

    auto ident = parser.expression();
    ASSERT_TRUE(ident.has_value());
    EXPECT_EQ(ident.value->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(ident.value)->value, "some_very_nice_expression"_sv);

    auto integer = parser.expression();
    ASSERT_TRUE(integer.has_value());
    EXPECT_EQ(integer.value->type, Expr::INTEGER_LIT);
    EXPECT_EQ(static_cast<ExprInteger*>(integer.value)->value, 999);
}

TEST(SQLParser, select_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "SELECT id, name FROM test;"_sv;
    Parser parser{&arena, source};

    auto select = parser.select_stmt();
    EXPECT_TRUE(select.has_value());

    auto select_stmt = select.value;

    EXPECT_EQ(select_stmt->exprs.count, 2);
    EXPECT_EQ(select_stmt->exprs[0]->type, Expr::IDENT);
    EXPECT_EQ(select_stmt->exprs[1]->type, Expr::IDENT);

    EXPECT_EQ(select_stmt->table->type, Expr::IDENT);
}

TEST(SQLParser, use_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "USE some_database;"_sv;
    Parser parser{&arena, source};

    auto use_stmt = parser.use_stmt();
    EXPECT_TRUE(use_stmt.has_value());

    EXPECT_EQ(use_stmt.value->database, "some_database"_sv);
}

TEST(SQLParser, insert_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "INSERT INTO Users (name, age) VALUES (oleh, some_age);"_sv;
    Parser parser{&arena, source};

    auto insert_stmt = parser.insert_stmt();
    EXPECT_TRUE(insert_stmt.has_value());

    EXPECT_EQ(insert_stmt.value->table->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(insert_stmt.value->table)->value, "Users"_sv);

    EXPECT_EQ(insert_stmt.value->columns.count, 2);
    EXPECT_EQ(insert_stmt.value->columns[0], "name"_sv);
    EXPECT_EQ(insert_stmt.value->columns[1], "age"_sv);

    EXPECT_EQ(insert_stmt.value->values.count, 2);

    auto values = insert_stmt.value->values.cast<ExprIdentifier*>();
    EXPECT_EQ(values[0]->value, "oleh"_sv);
    EXPECT_EQ(values[1]->value, "some_age"_sv);

    EXPECT_EQ(insert_stmt.value->values_counts.count, 1);
    EXPECT_EQ(insert_stmt.value->values_counts[0], 2);
}

TEST(SQLParser, update_stmt_with_filter) {
    ok::ArenaAllocator arena{};
    auto source = "UPDATE Users SET age = some_age WHERE true;"_sv;
    Parser parser{&arena, source};

    auto update_stmt = parser.update_stmt();
    ASSERT_TRUE(update_stmt.has_value());

    EXPECT_EQ(update_stmt.value->table->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(update_stmt.value->table)->value, "Users"_sv);

    EXPECT_EQ(update_stmt.value->columns.count, 1);
    EXPECT_EQ(update_stmt.value->columns[0], "age"_sv);

    EXPECT_EQ(update_stmt.value->values.count, 1);

    auto values = update_stmt.value->values.cast<ExprIdentifier*>();
    EXPECT_EQ(values[0]->value, "some_age"_sv);

    ASSERT_TRUE(update_stmt.value->filter.has_value());
    EXPECT_EQ(update_stmt.value->filter.value->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(update_stmt.value->filter.value)->value, "true"_sv);
}

TEST(SQLParser, update_stmt_without_filter) {
    ok::ArenaAllocator arena{};
    auto source = "UPDATE Users SET age = some_age;"_sv;
    Parser parser{&arena, source};

    auto update_stmt = parser.update_stmt();
    ASSERT_TRUE(update_stmt.has_value());

    EXPECT_EQ(update_stmt.value->table->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(update_stmt.value->table)->value, "Users"_sv);

    EXPECT_EQ(update_stmt.value->columns.count, 1);
    EXPECT_EQ(update_stmt.value->columns[0], "age"_sv);

    EXPECT_EQ(update_stmt.value->values.count, 1);

    auto values = update_stmt.value->values.cast<ExprIdentifier*>();
    EXPECT_EQ(values[0]->value, "some_age"_sv);

    EXPECT_FALSE(update_stmt.value->filter.has_value());
}

TEST(SQLParser, delete_stmt_with_filter) {
    ok::ArenaAllocator arena{};
    auto source = "DELETE FROM Users WHERE false;"_sv;
    Parser parser{&arena, source};

    auto delete_stmt = parser.delete_stmt();
    ASSERT_TRUE(delete_stmt.has_value());

    EXPECT_EQ(delete_stmt.value->table->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(delete_stmt.value->table)->value, "Users"_sv);

    ASSERT_TRUE(delete_stmt.value->filter.has_value());

    EXPECT_EQ(delete_stmt.value->filter.value->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(delete_stmt.value->filter.value)->value, "false"_sv);
}

TEST(SQLParser, delete_stmt_without_filter) {
    ok::ArenaAllocator arena{};
    auto source = "DELETE FROM Users;"_sv;
    Parser parser{&arena, source};

    auto delete_stmt = parser.delete_stmt();
    ASSERT_TRUE(delete_stmt.has_value());

    EXPECT_EQ(delete_stmt.value->table->type, Expr::IDENT);
    EXPECT_EQ(static_cast<ExprIdentifier*>(delete_stmt.value->table)->value, "Users"_sv);

    EXPECT_FALSE(delete_stmt.value->filter.has_value());
}

TEST(SQLParser, drop_database_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "DROP DATABASE MyDb;"_sv;
    Parser parser{&arena, source};

    auto drop_stmt = parser.drop_stmt();
    ASSERT_TRUE(drop_stmt.has_value());

    EXPECT_EQ(drop_stmt.value->target, DropStmt::Target::DATABASE);
    EXPECT_EQ(drop_stmt.value->name, "MyDb"_sv);
}

TEST(SQLParser, drop_table_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "DROP TABLE MyTable;"_sv;
    Parser parser{&arena, source};

    auto drop_stmt = parser.drop_stmt();
    ASSERT_TRUE(drop_stmt.has_value());

    EXPECT_EQ(drop_stmt.value->target, DropStmt::Target::TABLE);
    EXPECT_EQ(drop_stmt.value->name, "MyTable"_sv);
}

TEST(SQLParser, create_database_stmt) {
    ok::ArenaAllocator arena{};
    auto source = "CREATE DATABASE MyDb;"_sv;
    Parser parser{&arena, source};

    auto create_stmt = parser.create_stmt();
    ASSERT_TRUE(create_stmt.has_value());

    EXPECT_EQ(create_stmt.value->target, CreateStmt::Target::DATABASE);
    EXPECT_EQ(create_stmt.value->name, "MyDb"_sv);
}

TEST(SQLParser, create_table_stmt) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );)sql"_sv;
    Parser parser{&arena, source};

    auto create_stmt = parser.create_stmt();
    ASSERT_TRUE(create_stmt.has_value());

    EXPECT_EQ(create_stmt.value->target, CreateStmt::Target::TABLE);
    EXPECT_EQ(create_stmt.value->name, "MyTable"_sv);

    auto create_table = static_cast<CreateTableStmt*>(create_stmt.value);

    ASSERT_EQ(create_table->column_names.count, create_table->column_types.count);
    EXPECT_EQ(create_table->column_names.count, 2);

    EXPECT_EQ(create_table->column_names[0], "column1"_sv);
    EXPECT_EQ(create_table->column_names[1], "column2"_sv);

    EXPECT_EQ(create_table->column_types[0], "int"_sv);
    EXPECT_EQ(create_table->column_types[1], "text"_sv);
}

TEST(SQLParser, eof_error) {
    ok::ArenaAllocator arena{};
    auto source = ""_sv;
    Parser parser{&arena, source};

    EXPECT_TRUE(parser.is_eof());

    auto expr = parser.expression();
    EXPECT_FALSE(expr.has_value());

    EXPECT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    EXPECT_EQ(error.message, "unexpected EOF"_sv);

    auto expected_location = xmdb::SourceLocation{1, 1, 0};
    EXPECT_EQ(error.location, expected_location);
}

TEST(SQLParser, token_mismatch_error) {
    ok::ArenaAllocator arena{};
    auto source = "not_select error FROM parser_errors"_sv;
    Parser parser{&arena, source};

    auto opt_stmt = parser.select_stmt();
    EXPECT_FALSE(opt_stmt.has_value());
    EXPECT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    EXPECT_EQ(error.message, "expected a token of type SELECT, but got identifier instead"_sv);

    auto expected_location = xmdb::SourceLocation{1, 1, (uint32_t) strlen("not_select")};
    EXPECT_EQ(error.location, expected_location);
}
