#include <gtest/gtest.h>
#include "SQL/Parser.hpp"
#include "SQL/util.hpp"

using namespace xmdb::SQL;
using namespace ok::literals;

static const char *err(Parser p)
{
    return format_error(ok::temp_allocator(), p.error.get().location,
                        p.error.get().message.view())
            .cstr();
}

TEST(SQLParser, primary_expression)
{
    ok::ArenaAllocator arena{};
    auto source =
            R"(some_very_nice_expression 999 "a string literal!" TRUE FALSE NULL *)"_sv;
    Parser parser{&arena, source};

    auto ident = parser.expression();
    ASSERT_TRUE(ident.has_value());
    ASSERT_EQ(ident.value->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(ident.value)->value,
              "some_very_nice_expression"_sv);

    auto integer = parser.expression();
    ASSERT_TRUE(integer.has_value());
    ASSERT_EQ(integer.value->type, Expr::INTEGER_LIT);
    ASSERT_EQ(static_cast<IntegerExpr *>(integer.value)->value, 999);

    auto string = parser.expression();
    ASSERT_TRUE(string.has_value());
    ASSERT_EQ(string.value->type, Expr::STRING_LIT);
    ASSERT_EQ(static_cast<StringExpr *>(string.value)->value,
              "a string literal!"_sv);

    auto true_expr = parser.expression();
    ASSERT_TRUE(true_expr.has_value());
    ASSERT_EQ(true_expr.value->type, Expr::TRUE_LIT);

    auto false_expr = parser.expression();
    ASSERT_TRUE(false_expr.has_value());
    ASSERT_EQ(false_expr.value->type, Expr::FALSE_LIT);

    auto null_expr = parser.expression();
    ASSERT_TRUE(null_expr.has_value());
    ASSERT_EQ(null_expr.value->type, Expr::NULL_LIT);

    auto star_expr = parser.expression();
    ASSERT_TRUE(star_expr.has_value());
    ASSERT_EQ(star_expr.value->type, Expr::STAR);
}

TEST(SQLParser, binary_expression)
{
    ok::ArenaAllocator arena{};
    auto source = R"(1 > 2 5 < some_ident 4 = "some_string" 1 = 2 = 3)"_sv;
    Parser parser{&arena, source};

    auto gt = parser.expression();
    ASSERT_TRUE(gt.has_value());
    ASSERT_EQ(gt.value->type, Expr::BINARY_OP);

    auto gt_bin = static_cast<BinaryOpExpr *>(gt.value);
    ASSERT_EQ(gt_bin->kind, BinaryOpExpr::Kind::GT);
    ASSERT_EQ(gt_bin->lhs->type, Expr::INTEGER_LIT);
    ASSERT_EQ(gt_bin->rhs->type, Expr::INTEGER_LIT);

    auto lt = parser.expression();
    ASSERT_TRUE(lt.has_value());
    ASSERT_EQ(lt.value->type, Expr::BINARY_OP);

    auto lt_bin = static_cast<BinaryOpExpr *>(lt.value);
    ASSERT_EQ(lt_bin->kind, BinaryOpExpr::Kind::LT);
    ASSERT_EQ(lt_bin->lhs->type, Expr::INTEGER_LIT);
    ASSERT_EQ(lt_bin->rhs->type, Expr::IDENT);

    auto eq = parser.expression();
    ASSERT_TRUE(eq.has_value());
    ASSERT_EQ(eq.value->type, Expr::BINARY_OP);

    auto eq_bin = static_cast<BinaryOpExpr *>(eq.value);
    ASSERT_EQ(eq_bin->kind, BinaryOpExpr::Kind::EQ);
    ASSERT_EQ(eq_bin->lhs->type, Expr::INTEGER_LIT);
    ASSERT_EQ(eq_bin->rhs->type, Expr::STRING_LIT);

    auto eq_eq = parser.expression();
    ASSERT_TRUE(eq_eq.has_value());
    ASSERT_EQ(eq_eq.value->type, Expr::BINARY_OP);

    auto eq_eq_bin = static_cast<BinaryOpExpr *>(eq_eq.value);
    ASSERT_EQ(eq_eq_bin->kind, BinaryOpExpr::Kind::EQ);
    ASSERT_EQ(eq_eq_bin->lhs->type, Expr::INTEGER_LIT);
    ASSERT_EQ(eq_eq_bin->rhs->type, Expr::BINARY_OP);

    auto eq_eq_rhs = static_cast<BinaryOpExpr *>(eq_eq_bin->rhs);
    ASSERT_EQ(eq_eq_rhs->kind, BinaryOpExpr::Kind::EQ);
    ASSERT_EQ(eq_eq_rhs->lhs->type, Expr::INTEGER_LIT);
    ASSERT_EQ(eq_eq_rhs->rhs->type, Expr::INTEGER_LIT);
}

TEST(SQLParser, select_expr)
{
    ok::ArenaAllocator arena{};
    auto source = "SELECT id, name FROM test;"_sv;
    Parser parser{&arena, source};

    auto select = parser.expression();
    ASSERT_TRUE(select.has_value());
    ASSERT_TRUE(select.value->type == Expr::SELECT);

    auto select_expr = static_cast<SelectExpr *>(select.value);

    ASSERT_EQ(select_expr->exprs.count, 2);
    ASSERT_EQ(select_expr->exprs[0]->type, Expr::IDENT);
    ASSERT_EQ(select_expr->exprs[1]->type, Expr::IDENT);

    ASSERT_EQ(select_expr->table->type, Expr::IDENT);
}

TEST(SQLParser, select_star_expr)
{
    ok::ArenaAllocator arena{};
    auto source = "SELECT * FROM my_table;"_sv;
    Parser parser{&arena, source};

    auto select = parser.expression();
    ASSERT_TRUE(select.has_value())
            << format_error(&arena, parser.error.get().location,
                            parser.error.get().message.view())
                       .cstr();
    ASSERT_TRUE(select.value->type == Expr::SELECT);

    auto select_expr = static_cast<SelectExpr *>(select.value);

    ASSERT_EQ(select_expr->exprs.count, 1);
    ASSERT_EQ(select_expr->exprs[0]->type, Expr::STAR);

    ASSERT_EQ(select_expr->table->type, Expr::IDENT);
}

TEST(SQLParser, use_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "USE some_database;"_sv;
    Parser parser{&arena, source};

    auto use_stmt = parser.use_stmt();
    ASSERT_TRUE(use_stmt.has_value());

    ASSERT_EQ(use_stmt.value->database, "some_database"_sv);
}

TEST(SQLParser, insert_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "INSERT INTO Users (name, age) VALUES (oleh, some_age);"_sv;
    Parser parser{&arena, source};

    auto insert_stmt = parser.insert_stmt();
    ASSERT_TRUE(insert_stmt.has_value())
            << format_error(&arena, parser.error.get().location,
                            parser.error.get().message.view())
                       .cstr();

    ASSERT_EQ(insert_stmt.value->table_name, "Users"_sv);

    ASSERT_EQ(insert_stmt.value->columns.count, 2);
    ASSERT_EQ(insert_stmt.value->columns[0], "name"_sv);
    ASSERT_EQ(insert_stmt.value->columns[1], "age"_sv);

    ASSERT_EQ(insert_stmt.value->values.count, 2);

    auto values = insert_stmt.value->values.cast<IdentifierExpr *>();
    ASSERT_EQ(values[0]->value, "oleh"_sv);
    ASSERT_EQ(values[1]->value, "some_age"_sv);

    ASSERT_EQ(insert_stmt.value->values_counts.count, 1);
    ASSERT_EQ(insert_stmt.value->values_counts[0], 2);
}

TEST(SQLParser, update_stmt_with_filter)
{
    ok::ArenaAllocator arena{};
    auto source = "UPDATE Users SET age = some_age WHERE true;"_sv;
    Parser parser{&arena, source};

    auto update_stmt = parser.update_stmt();
    ASSERT_TRUE(update_stmt.has_value());

    ASSERT_EQ(update_stmt.value->table->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(update_stmt.value->table)->value,
              "Users"_sv);

    ASSERT_EQ(update_stmt.value->columns.count, 1);
    ASSERT_EQ(update_stmt.value->columns[0], "age"_sv);

    ASSERT_EQ(update_stmt.value->values.count, 1);

    auto values = update_stmt.value->values.cast<IdentifierExpr *>();
    ASSERT_EQ(values[0]->value, "some_age"_sv);

    ASSERT_TRUE(update_stmt.value->filter.has_value());
    ASSERT_EQ(update_stmt.value->filter.value->type, Expr::TRUE_LIT);
}

TEST(SQLParser, update_stmt_without_filter)
{
    ok::ArenaAllocator arena{};
    auto source = "UPDATE Users SET age = some_age;"_sv;
    Parser parser{&arena, source};

    auto update_stmt = parser.update_stmt();
    ASSERT_TRUE(update_stmt.has_value());

    ASSERT_EQ(update_stmt.value->table->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(update_stmt.value->table)->value,
              "Users"_sv);

    ASSERT_EQ(update_stmt.value->columns.count, 1);
    ASSERT_EQ(update_stmt.value->columns[0], "age"_sv);

    ASSERT_EQ(update_stmt.value->values.count, 1);

    auto values = update_stmt.value->values.cast<IdentifierExpr *>();
    ASSERT_EQ(values[0]->value, "some_age"_sv);

    ASSERT_FALSE(update_stmt.value->filter.has_value());
}

TEST(SQLParser, delete_stmt_with_filter)
{
    ok::ArenaAllocator arena{};
    auto source = "DELETE FROM Users WHERE false;"_sv;
    Parser parser{&arena, source};

    auto delete_stmt = parser.delete_stmt();
    ASSERT_TRUE(delete_stmt.has_value());

    ASSERT_EQ(delete_stmt.value->table->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(delete_stmt.value->table)->value,
              "Users"_sv);

    ASSERT_TRUE(delete_stmt.value->filter.has_value());

    ASSERT_EQ(delete_stmt.value->filter.value->type, Expr::FALSE_LIT);
}

TEST(SQLParser, delete_stmt_without_filter)
{
    ok::ArenaAllocator arena{};
    auto source = "DELETE FROM Users;"_sv;
    Parser parser{&arena, source};

    auto delete_stmt = parser.delete_stmt();
    ASSERT_TRUE(delete_stmt.has_value());

    ASSERT_EQ(delete_stmt.value->table->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(delete_stmt.value->table)->value,
              "Users"_sv);

    ASSERT_FALSE(delete_stmt.value->filter.has_value());
}

TEST(SQLParser, drop_database_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "DROP DATABASE MyDb;"_sv;
    Parser parser{&arena, source};

    auto drop_stmt = parser.drop_stmt();
    ASSERT_TRUE(drop_stmt.has_value());

    ASSERT_EQ(drop_stmt.value->target, DropStmt::Target::DATABASE);
    ASSERT_EQ(drop_stmt.value->name, "MyDb"_sv);
}

TEST(SQLParser, drop_table_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "DROP TABLE MyTable;"_sv;
    Parser parser{&arena, source};

    auto drop_stmt = parser.drop_stmt();
    ASSERT_TRUE(drop_stmt.has_value());

    ASSERT_EQ(drop_stmt.value->target, DropStmt::Target::TABLE);
    ASSERT_EQ(drop_stmt.value->name, "MyTable"_sv);
}

TEST(SQLParser, create_database_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "CREATE DATABASE MyDb;"_sv;
    Parser parser{&arena, source};

    auto create_stmt = parser.create_stmt();
    ASSERT_TRUE(create_stmt.has_value());

    ASSERT_EQ(create_stmt.value->target, CreateStmt::Target::DATABASE);
    ASSERT_EQ(create_stmt.value->name, "MyDb"_sv);
}

TEST(SQLParser, create_table_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );)sql"_sv;
    Parser parser{&arena, source};

    auto create_stmt = parser.create_stmt();
    ASSERT_TRUE(create_stmt.has_value());

    ASSERT_EQ(create_stmt.value->target, CreateStmt::Target::TABLE);
    ASSERT_EQ(create_stmt.value->name, "MyTable"_sv);

    auto create_table = static_cast<CreateTableStmt *>(create_stmt.value);

    ASSERT_EQ(create_table->column_names.count,
              create_table->column_types.count);
    ASSERT_EQ(create_table->column_names.count, 2);

    ASSERT_EQ(create_table->column_names[0], "column1"_sv);
    ASSERT_EQ(create_table->column_names[1], "column2"_sv);

    ASSERT_EQ(create_table->column_types[0], "int"_sv);
    ASSERT_EQ(create_table->column_types[1], "text"_sv);
}

TEST(SQLParser, create_user_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "create user usr;"_sv;
    Parser parser{&arena, source};

    auto create_stmt = parser.create_stmt();
    ASSERT_TRUE(create_stmt.has_value());

    ASSERT_EQ(create_stmt.value->target, CreateStmt::Target::USER);
    ASSERT_EQ(create_stmt.value->name, "usr"_sv);
}

TEST(SQLParser, set_clause)
{
    ok::ArenaAllocator arena{};
    auto source = "foo = bar;"_sv;
    Parser parser{&arena, source};

    auto clause = parser.set_clause();
    ASSERT_TRUE(clause.has_value());

    ASSERT_EQ(clause.value.name, "foo"_sv);
    ASSERT_EQ(clause.value.value->type, Expr::Type::IDENT);

    auto ident = static_cast<IdentifierExpr *>(clause.value.value);

    ASSERT_EQ(ident->value, "bar"_sv);
}

TEST(SQLParser, alter_user_stmt)
{
    ok::ArenaAllocator arena{};
    auto source = "alter user foo set name = 'bar', password = '***';"_sv;
    Parser parser{&arena, source};

    auto alter_stmt = parser.stmt();
    ASSERT_TRUE(alter_stmt.has_value());

    auto alter = static_cast<AlterStmt *>(alter_stmt.value);

    ASSERT_EQ(alter->target, AlterStmt::Target::USER);

    auto alter_user = static_cast<AlterUserStmt *>(alter);

    ASSERT_EQ(alter_user->user_name, "foo"_sv);
    ASSERT_EQ(alter_user->set_clauses.count, 2);

    auto first_clause = alter_user->set_clauses[0];
    auto second_clause = alter_user->set_clauses[1];

    ASSERT_EQ(first_clause.name, "name"_sv);
    ASSERT_EQ(first_clause.value->type, Expr::STRING_LIT);
    ASSERT_EQ(static_cast<StringExpr *>(first_clause.value)->value, "bar"_sv);

    ASSERT_EQ(second_clause.name, "password"_sv);
    ASSERT_EQ(second_clause.value->type, Expr::STRING_LIT);
    ASSERT_EQ(static_cast<StringExpr *>(second_clause.value)->value, "***"_sv);
}

TEST(SQLParser, call_expr)
{
    ok::ArenaAllocator arena{};
    auto source = "some_func(arg1, 1337)"_sv;
    Parser parser{&arena, source};

    auto call_expr = parser.expression();
    ASSERT_TRUE(call_expr) << err(parser);

    auto *call = static_cast<CallExpr *>(call_expr.get());
    ASSERT_EQ(call->fn->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(call->fn)->value, "some_func"_sv);

    ASSERT_EQ(call->args.count, 2);

    ASSERT_EQ(call->args[0]->type, Expr::IDENT);
    ASSERT_EQ(static_cast<IdentifierExpr *>(call->args[0])->value, "arg1"_sv);

    ASSERT_EQ(call->args[1]->type, Expr::INTEGER_LIT);
    ASSERT_EQ(static_cast<IntegerExpr *>(call->args[1])->value, 1337);
}

TEST(SQLParser, query)
{
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    SELECT column1 FROM MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());
    ASSERT_EQ(query.value.stmts.count, 2);
    ASSERT_EQ(query.value.stmts[0]->type, Stmt::CREATE);
    ASSERT_EQ(query.value.stmts[1]->type, Stmt::EXPR);

    auto select_expr = static_cast<ExprStmt *>(query.value.stmts[1]);
    ASSERT_EQ(select_expr->expr->type, Expr::SELECT);
}

TEST(SQLParser, eof_error)
{
    ok::ArenaAllocator arena{};
    auto source = ""_sv;
    Parser parser{&arena, source};

    ASSERT_TRUE(parser.is_eof());

    auto expr = parser.expression();
    ASSERT_FALSE(expr.has_value());

    ASSERT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    ASSERT_EQ(error.message, "unexpected EOF"_sv);

    auto expected_location = xmdb::SourceLocation{1, 1, 0};
    ASSERT_EQ(error.location, expected_location);
}

TEST(SQLParser, token_mismatch_error)
{
    ok::ArenaAllocator arena{};
    auto source = "not_select error FROM parser_errors"_sv;
    Parser parser{&arena, source};

    auto opt_stmt = parser.delete_stmt();
    ASSERT_FALSE(opt_stmt.has_value());
    ASSERT_TRUE(parser.error.has_value());

    auto error = parser.error.value;
    ASSERT_EQ(error.message,
              "expected a token of type DELETE, but got identifier instead"_sv);

    auto expected_location =
            xmdb::SourceLocation{1, 1, (uint32_t) strlen("not_select")};
    ASSERT_EQ(error.location, expected_location);
}
