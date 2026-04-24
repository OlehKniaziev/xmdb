// TODO(oleh): Refactor the constructors in this file to accept strings as
// 'StringView'.

#ifndef XMDB_AST_H_
#define XMDB_AST_H_

#include <Core/ok.hpp>
#include "Lexer.hpp"

namespace xmdb::SQL
{
/**
 * @brief Base class for all SQL statements in the AST.
 */
struct Stmt
{
    /**
     * @brief Types of SQL statements.
     */
    enum Type : U8
    {
        USE, ///< USE statement.
        INSERT, ///< INSERT statement.
        UPDATE, ///< UPDATE statement.
        DELETE, ///< DELETE statement.
        DROP, ///< DROP statement.
        CREATE, ///< CREATE statement.
        EXPR, ///< Expression statement.
        ALTER, ///< ALTER statement.
    };

    Type type; ///< The type of the statement.
    Token token; ///< The primary token associated with the statement.
};

/**
 * @brief Base class for all SQL expressions in the AST.
 */
struct Expr
{
    /**
     * @brief Types of SQL expressions.
     */
    enum Type : U8
    {
        IDENT, ///< Identifier.
        INTEGER_LIT, ///< Integer literal.
        STRING_LIT, ///< String literal.
        TRUE_LIT, ///< Boolean TRUE literal.
        FALSE_LIT, ///< Boolean FALSE literal.
        NULL_LIT, ///< NULL literal.
        BINARY_OP, ///< Binary operation.
        SELECT, ///< SELECT expression.
        STAR, ///< Star ('*') expression.
        CALL, ///< Function call expression.
    };

    /**
     * @brief Allocates a TRUE literal expression.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @return Pointer to the new Expr.
     */
    static Expr *alloc_true(ok::Allocator *allocator, Token token)
    {
        Expr *expr = allocator->alloc<Expr>();
        expr->type = TRUE_LIT;
        expr->token = token;
        return expr;
    }

    /**
     * @brief Allocates a FALSE literal expression.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @return Pointer to the new Expr.
     */
    static Expr *alloc_false(ok::Allocator *allocator, Token token)
    {
        Expr *expr = allocator->alloc<Expr>();
        expr->type = FALSE_LIT;
        expr->token = token;
        return expr;
    }

    /**
     * @brief Allocates a NULL literal expression.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @return Pointer to the new Expr.
     */
    static Expr *alloc_null(ok::Allocator *allocator, Token token)
    {
        Expr *expr = allocator->alloc<Expr>();
        expr->type = NULL_LIT;
        expr->token = token;
        return expr;
    }

    bool operator==(const Expr &other) const;

    /**
     * @brief Converts the expression to a string representation.
     * @param allocator The allocator to use for the string.
     * @return The string representation.
     */
    ok::String to_string(ok::Allocator *allocator) const;

    Type type; ///< The type of the expression.
    Token token; ///< The primary token of the expression.
};

/**
 * @brief Computes a hash value for the expression.
 * @param expr The expression.
 * @return The hash value.
 */
U64 ok_hash_value(const Expr &expr);

/**
 * @brief Represents an identifier expression (e.g. column name).
 */
struct IdentifierExpr : public Expr
{
    /**
     * @brief Allocates a new IdentifierExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param value The identifier string.
     * @return Pointer to the new IdentifierExpr.
     */
    static IdentifierExpr *alloc(ok::Allocator *allocator, Token token,
                                 ok::StringView value)
    {
        auto *expr = allocator->alloc<IdentifierExpr>();
        expr->type = IDENT;
        expr->token = token;
        expr->value = value.to_string(allocator);
        return expr;
    }

    ok::String value; ///< The identifier value.
};

/**
 * @brief Represents an integer literal expression.
 */
struct IntegerExpr : public Expr
{
    /**
     * @brief Allocates a new IntegerExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param value The integer value.
     * @return Pointer to the new IntegerExpr.
     */
    static IntegerExpr *alloc(ok::Allocator *allocator, Token token,
                              int64_t value)
    {
        auto *expr = allocator->alloc<IntegerExpr>();
        expr->type = INTEGER_LIT;
        expr->token = token;
        expr->value = value;
        return expr;
    }

    int64_t value; ///< The integer value.
};

/**
 * @brief Represents a string literal expression.
 */
struct StringExpr : public Expr
{
    /**
     * @brief Allocates a new StringExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param value The string value.
     * @return Pointer to the new StringExpr.
     */
    static StringExpr *alloc(ok::Allocator *allocator, Token token,
                             ok::String value)
    {
        auto *expr = allocator->alloc<StringExpr>();
        expr->type = STRING_LIT;
        expr->token = token;
        expr->value = value;
        return expr;
    }

    ok::String value; ///< The string value.
};

/**
 * @brief Represents a binary operation expression.
 */
struct BinaryOpExpr : public Expr
{
    /**
     * @brief Kinds of binary operations.
     */
    enum class Kind : U8
    {
        EQ, ///< Equality.
        GT, ///< Greater than.
        LT, ///< Less than.
    };

    /**
     * @brief Allocates a new BinaryOpExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param kind The kind of binary operation.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @return Pointer to the new BinaryOpExpr.
     */
    static BinaryOpExpr *alloc(ok::Allocator *allocator, Token token, Kind kind,
                               Expr *lhs, Expr *rhs)
    {
        auto *expr = allocator->alloc<BinaryOpExpr>();
        expr->type = BINARY_OP;
        expr->token = token;
        expr->kind = kind;
        expr->lhs = lhs;
        expr->rhs = rhs;
        return expr;
    }

    Kind kind; ///< The kind of binary operation.
    Expr *lhs; ///< Left-hand side operand.
    Expr *rhs; ///< Right-hand side operand.
};

/**
 * @brief Represents a star ('*') expression.
 */
struct StarExpr : public Expr
{
    /**
     * @brief Allocates a new StarExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @return Pointer to the new StarExpr.
     */
    static StarExpr *alloc(ok::Allocator *allocator, Token token)
    {
        auto *expr = allocator->alloc<StarExpr>();
        expr->type = STAR;
        expr->token = token;
        return expr;
    }
};

/**
 * @brief Represents a SELECT expression (subquery).
 */
struct SelectExpr : public Expr
{
    /**
     * @brief Allocates a new SelectExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param exprs The expressions being selected.
     * @param table The source table expression.
     * @return Pointer to the new SelectExpr.
     */
    static SelectExpr *alloc(ok::Allocator *allocator, Token token,
                             ok::Slice<Expr *> exprs, Expr *table)
    {
        auto *expr = allocator->alloc<SelectExpr>();
        expr->type = SELECT;
        expr->token = token;
        expr->exprs = exprs;
        expr->table = table;
        return expr;
    }

    ok::Slice<Expr *> exprs; ///< The expressions being selected.
    Expr *table; ///< The source table.
};

/**
 * @brief Represents a function call expression.
 */
struct CallExpr : public Expr
{
    /**
     * @brief Allocates a new CallExpr.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param fn The function expression.
     * @param args The arguments to the function call.
     * @return Pointer to the new CallExpr.
     */
    static CallExpr *alloc(ok::Allocator *allocator, Token token, Expr *fn,
                           ok::Slice<Expr *> args)
    {
        auto *expr = allocator->alloc<CallExpr>();
        expr->type = CALL;
        expr->token = token;
        expr->fn = fn;
        expr->args = args;
        return expr;
    }

    Expr *fn; ///< The function being called.
    ok::Slice<Expr *> args; ///< The arguments to the function call.
};

/**
 * @brief Represents an expression statement.
 */
struct ExprStmt : public Stmt
{
    /**
     * @brief Allocates a new ExprStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param expr The expression.
     * @return Pointer to the new ExprStmt.
     */
    static ExprStmt *alloc(ok::Allocator *allocator, Token token, Expr *expr)
    {
        auto *stmt = allocator->alloc<ExprStmt>();
        stmt->type = EXPR;
        stmt->token = token;
        stmt->expr = expr;
        return stmt;
    }

    Expr *expr; ///< The expression.
};

/**
 * @brief Represents a USE statement.
 */
struct UseStmt : public Stmt
{
    /**
     * @brief Allocates a new UseStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param database The name of the database.
     * @return Pointer to the new UseStmt.
     */
    static UseStmt *alloc(ok::Allocator *allocator, Token token,
                          ok::StringView database)
    {
        auto *stmt = allocator->alloc<UseStmt>();
        stmt->type = USE;
        stmt->token = token;
        stmt->database = database.to_string(allocator);
        return stmt;
    }

    ok::String database; ///< The name of the database to use.
};

/**
 * @brief Represents an INSERT statement.
 */
struct InsertStmt : public Stmt
{
    /**
     * @brief Allocates a new InsertStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param table_name The name of the table.
     * @param columns The names of the columns.
     * @param values The values to insert.
     * @param values_counts The number of values per row.
     * @return Pointer to the new InsertStmt.
     */
    static InsertStmt *alloc(ok::Allocator *allocator, Token token,
                             ok::StringView table_name,
                             ok::Slice<ok::String> columns,
                             ok::Slice<Expr *> values,
                             ok::Slice<uint32_t> values_counts)
    {
        auto *stmt = allocator->alloc<InsertStmt>();
        stmt->type = INSERT;
        stmt->token = token;
        stmt->table_name = table_name.to_string(allocator);
        stmt->columns = columns;
        stmt->values = values;
        stmt->values_counts = values_counts;
        return stmt;
    }

    ok::String table_name; ///< The name of the table to insert into.
    ok::Slice<ok::String> columns; ///< The columns to insert values for.
    ok::Slice<Expr *> values; ///< The values to insert.
    ok::Slice<uint32_t> values_counts; ///< The number of values in each row.
};

/**
 * @brief Represents an UPDATE statement.
 */
struct UpdateStmt : public Stmt
{
    /**
     * @brief Allocates a new UpdateStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param table The table expression.
     * @param columns The columns to update.
     * @param values The new values.
     * @param filter The filter expression.
     * @return Pointer to the new UpdateStmt.
     */
    static UpdateStmt *alloc(ok::Allocator *allocator, Token token, Expr *table,
                             ok::Slice<ok::String> columns,
                             ok::Slice<Expr *> values,
                             ok::Optional<Expr *> filter)
    {
        auto *stmt = allocator->alloc<UpdateStmt>();
        stmt->type = UPDATE;
        stmt->token = token;
        stmt->table = table;
        stmt->columns = columns;
        stmt->values = values;
        stmt->filter = filter;
        return stmt;
    }

    Expr *table; ///< The table to update.
    ok::Slice<ok::String> columns; ///< The columns to update.
    ok::Slice<Expr *> values; ///< The new values.
    ok::Optional<Expr *> filter; ///< The WHERE clause filter.
};

/**
 * @brief Represents a DELETE statement.
 */
struct DeleteStmt : public Stmt
{
    /**
     * @brief Allocates a new DeleteStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param table The table expression.
     * @param filter The filter expression.
     * @return Pointer to the new DeleteStmt.
     */
    static DeleteStmt *alloc(ok::Allocator *allocator, Token token, Expr *table,
                             ok::Optional<Expr *> filter)
    {
        auto *stmt = allocator->alloc<DeleteStmt>();
        stmt->type = DELETE;
        stmt->token = token;
        stmt->table = table;
        stmt->filter = filter;
        return stmt;
    }

    Expr *table; ///< The table to delete from.
    ok::Optional<Expr *> filter; ///< The WHERE clause filter.
};

/**
 * @brief Represents a DROP statement.
 */
struct DropStmt : public Stmt
{
    /**
     * @brief Targets for DROP statement.
     */
    enum class Target : U8
    {
        TABLE, ///< DROP TABLE.
        DATABASE, ///< DROP DATABASE.
    };

    /**
     * @brief Allocates a new DropStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param target The target of the DROP operation.
     * @param name The name of the target.
     * @return Pointer to the new DropStmt.
     */
    static DropStmt *alloc(ok::Allocator *allocator, Token token, Target target,
                           ok::String name)
    {
        auto *stmt = allocator->alloc<DropStmt>();
        stmt->type = DROP;
        stmt->token = token;
        stmt->target = target;
        stmt->name = name;
        return stmt;
    }

    Target target; ///< The target of the DROP operation.
    ok::String name; ///< The name of the target.
};

/**
 * @brief Base class for CREATE statements.
 */
struct CreateStmt : public Stmt
{
    /**
     * @brief Targets for CREATE statement.
     */
    enum class Target : U8
    {
        DATABASE, ///< CREATE DATABASE.
        TABLE, ///< CREATE TABLE.
        USER, ///< CREATE USER.
    };

    Target target; ///< The target of the CREATE operation.
    ok::String name; ///< The name of the target.
};

/**
 * @brief Represents a CREATE DATABASE statement.
 */
struct CreateDatabaseStmt : public CreateStmt
{
    /**
     * @brief Allocates a new CreateDatabaseStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param name The name of the database.
     * @return Pointer to the new CreateDatabaseStmt.
     */
    static CreateDatabaseStmt *alloc(ok::Allocator *allocator, Token token,
                                     ok::String name)
    {
        auto *stmt = allocator->alloc<CreateDatabaseStmt>();
        stmt->type = CREATE;
        stmt->token = token;
        stmt->target = Target::DATABASE;
        stmt->name = name;
        return stmt;
    }
};

/**
 * @brief Represents a CREATE TABLE statement.
 */
struct CreateTableStmt : public CreateStmt
{
    /**
     * @brief Allocates a new CreateTableStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param name The name of the table.
     * @param column_names The names of the columns.
     * @param column_types The types of the columns.
     * @return Pointer to the new CreateTableStmt.
     */
    static CreateTableStmt *alloc(ok::Allocator *allocator, Token token,
                                  ok::String name,
                                  ok::Slice<ok::String> column_names,
                                  ok::Slice<ok::String> column_types)
    {
        auto *stmt = allocator->alloc<CreateTableStmt>();
        stmt->type = CREATE;
        stmt->token = token;
        stmt->target = Target::TABLE;
        stmt->name = name;
        stmt->column_names = column_names;
        stmt->column_types = column_types;
        return stmt;
    }

    ok::Slice<ok::String> column_names; ///< The names of the columns.
    ok::Slice<ok::String> column_types; ///< The types of the columns.
};

/**
 * @brief Represents a CREATE USER statement.
 */
struct CreateUserStmt : public CreateStmt
{
    /**
     * @brief Allocates a new CreateUserStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param name The name of the user.
     * @return Pointer to the new CreateUserStmt.
     */
    static CreateUserStmt *alloc(ok::Allocator *allocator, Token token,
                                 ok::String name)
    {
        auto *stmt = allocator->alloc<CreateUserStmt>();
        stmt->type = CREATE;
        stmt->target = Target::USER;
        stmt->token = token;
        stmt->name = name;
        return stmt;
    }
};

/**
 * @brief Represents a single assignment in a SET clause.
 */
struct SetClause
{
    ok::String name; ///< The name of the field.
    Expr *value; ///< The value to assign.
};

/**
 * @brief Base class for ALTER statements.
 */
struct AlterStmt : public Stmt
{
    /**
     * @brief Targets for ALTER statement.
     */
    enum class Target : U8
    {
        USER, ///< ALTER USER.
    };

    Target target; ///< The target of the ALTER operation.
};

/**
 * @brief Represents an ALTER USER statement.
 */
struct AlterUserStmt : public AlterStmt
{
    /**
     * @brief Allocates a new AlterUserStmt.
     * @param allocator The allocator to use.
     * @param token The associated token.
     * @param name The name of the user.
     * @param set_clauses The changes to apply.
     * @return Pointer to the new AlterUserStmt.
     */
    static AlterUserStmt *alloc(ok::Allocator *allocator, Token token,
                                ok::StringView name,
                                ok::Slice<SetClause> set_clauses)
    {
        auto *stmt = allocator->alloc<AlterUserStmt>();
        stmt->type = ALTER;
        stmt->target = Target::USER;
        stmt->token = token;
        stmt->user_name = name.to_string(allocator);
        stmt->set_clauses = set_clauses;
        return stmt;
    }

    ok::String user_name; ///< The name of the user to alter.
    ok::Slice<SetClause> set_clauses; ///< The changes to apply.
};

/**
 * @brief Represents a full SQL query.
 */
struct Query
{
    ok::Slice<Stmt *> stmts; ///< The statements in the query.
};
}; // namespace xmdb::SQL

#endif // XMDB_AST_H_
