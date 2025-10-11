#ifndef XMDB_AST_H_
#define XMDB_AST_H_

#include <Core/ok.hpp>
#include "Lexer.hpp"

namespace xmdb::SQL {
struct Stmt {
    enum Type {
        SELECT,
        USE,
        INSERT,
        UPDATE,
        DELETE,
        DROP,
        CREATE,
        EXPR,
    };

    Type type;
    Token token;
};

struct Expr {
    enum Type : uint8_t {
        IDENT,
        INTEGER_LIT,
        STRING_LIT,
        TRUE_LIT,
        FALSE_LIT,
        NULL_LIT,
        BINARY_OP,
        SELECT,
        STAR,
    };

    static Expr* alloc_true(ok::Allocator* allocator, Token token) {
        Expr* expr = allocator->alloc<Expr>();
        expr->type = TRUE_LIT;
        expr->token = token;
        return expr;
    }

    static Expr* alloc_false(ok::Allocator* allocator, Token token) {
        Expr* expr = allocator->alloc<Expr>();
        expr->type = FALSE_LIT;
        expr->token = token;
        return expr;
    }

    static Expr* alloc_null(ok::Allocator* allocator, Token token) {
        Expr* expr = allocator->alloc<Expr>();
        expr->type = NULL_LIT;
        expr->token = token;
        return expr;
    }

    U64 ok_hash_value() const;

    bool operator ==(const Expr&) const;

    ok::String to_string(ok::Allocator*) const;

    Type type;
    Token token;
};

struct IdentifierExpr : public Expr {
    static IdentifierExpr* alloc(ok::Allocator* allocator, Token token, ok::StringView value) {
        auto* expr = allocator->alloc<IdentifierExpr>();
        expr->type = IDENT;
        expr->token = token;
        expr->value = value.to_string(allocator);
        return expr;
    }

    ok::String value;
};

struct IntegerExpr : public Expr {
    static IntegerExpr* alloc(ok::Allocator* allocator, Token token, int64_t value) {
        auto* expr = allocator->alloc<IntegerExpr>();
        expr->type = INTEGER_LIT;
        expr->token = token;
        expr->value = value;
        return expr;
    }

    int64_t value;
};

struct StringExpr : public Expr {
    static StringExpr* alloc(ok::Allocator* allocator, Token token, ok::String value) {
        auto* expr = allocator->alloc<StringExpr>();
        expr->type = STRING_LIT;
        expr->token = token;
        expr->value = value;
        return expr;
    }

    ok::String value;
};

struct BinaryOpExpr : public Expr {
    enum class Kind : uint8_t {
        EQ,
        GT,
        LT,
    };

    static BinaryOpExpr* alloc(ok::Allocator* allocator, Token token, Kind kind, Expr* lhs, Expr* rhs) {
        auto* expr = allocator->alloc<BinaryOpExpr>();
        expr->type = BINARY_OP;
        expr->token = token;
        expr->kind = kind;
        expr->lhs = lhs;
        expr->rhs = rhs;
        return expr;
    }

    Kind kind;
    Expr* lhs;
    Expr* rhs;
};

struct StarExpr : public Expr {
    static StarExpr *alloc(ok::Allocator *allocator, Token token) {
        auto *expr = allocator->alloc<StarExpr>();
        expr->type = STAR;
        expr->token = token;
        return expr;
    }
};

struct SelectExpr : public Expr {
    static SelectExpr* alloc(ok::Allocator* allocator, Token token, ok::Slice<Expr*> exprs, Expr* table) {
        auto* expr = allocator->alloc<SelectExpr>();
        expr->type = SELECT;
        expr->token = token;
        expr->exprs = exprs;
        expr->table = table;
        return expr;
    }

    ok::Slice<Expr*> exprs;
    Expr* table;
};

struct ExprStmt : public Stmt {
    static ExprStmt* alloc(ok::Allocator* allocator, Token token, Expr* expr) {
        auto* stmt = allocator->alloc<ExprStmt>();
        stmt->type = EXPR;
        stmt->token = token;
        stmt->expr = expr;
        return stmt;
    }

    Expr* expr;
};

struct UseStmt : public Stmt {
    static UseStmt* alloc(ok::Allocator* allocator, Token token, ok::StringView database) {
        auto* stmt = allocator->alloc<UseStmt>();
        stmt->type = USE;
        stmt->token = token;
        stmt->database = database.to_string(allocator);
        return stmt;
    }

    ok::String database;
};

struct InsertStmt : public Stmt {
    static InsertStmt* alloc(ok::Allocator* allocator, Token token, Expr* table, ok::Slice<ok::String> columns,
                                ok::Slice<Expr*> values, ok::Slice<uint32_t> values_counts) {
        auto* stmt = allocator->alloc<InsertStmt>();
        stmt->type = INSERT;
        stmt->token = token;
        stmt->table = table;
        stmt->columns = columns;
        stmt->values = values;
        stmt->values_counts = values_counts;
        return stmt;
    }

    Expr* table;
    ok::Slice<ok::String> columns;
    ok::Slice<Expr*> values;
    ok::Slice<uint32_t> values_counts;
};

struct UpdateStmt : public Stmt {
    static UpdateStmt* alloc(ok::Allocator* allocator, Token token, Expr* table, ok::Slice<ok::String> columns,
                                ok::Slice<Expr*> values, ok::Optional<Expr*> filter) {
        auto* stmt = allocator->alloc<UpdateStmt>();
        stmt->type = UPDATE;
        stmt->token = token;
        stmt->table = table;
        stmt->columns = columns;
        stmt->values = values;
        stmt->filter = filter;
        return stmt;
    }

    Expr* table;
    ok::Slice<ok::String> columns;
    ok::Slice<Expr*> values;
    ok::Optional<Expr*> filter;
};

struct DeleteStmt : public Stmt {
    static DeleteStmt* alloc(ok::Allocator* allocator, Token token, Expr* table, ok::Optional<Expr*> filter) {
        auto* stmt = allocator->alloc<DeleteStmt>();
        stmt->type = DELETE;
        stmt->token = token;
        stmt->table = table;
        stmt->filter = filter;
        return stmt;
    }

    Expr* table;
    ok::Optional<Expr*> filter;
};

struct DropStmt : public Stmt {
    enum class Target : uint8_t {
        TABLE,
        DATABASE,
    };

    static DropStmt* alloc(ok::Allocator* allocator, Token token, Target target, ok::String name) {
        auto* stmt = allocator->alloc<DropStmt>();
        stmt->type = DROP;
        stmt->token = token;
        stmt->target = target;
        stmt->name = name;
        return stmt;
    }

    Target target;
    ok::String name;
};

struct CreateStmt : public Stmt {
    enum class Target : uint8_t {
        DATABASE,
        TABLE,
        USER,
    };

    Target target;
    ok::String name;
};

struct CreateDatabaseStmt : public CreateStmt {
    static CreateDatabaseStmt* alloc(ok::Allocator* allocator, Token token, ok::String name) {
        auto* stmt = allocator->alloc<CreateDatabaseStmt>();
        stmt->type = CREATE;
        stmt->token = token;
        stmt->target = Target::DATABASE;
        stmt->name = name;
        return stmt;
    }
};

struct CreateTableStmt : public CreateStmt {
    static CreateTableStmt* alloc(ok::Allocator* allocator, Token token, ok::String name, ok::Slice<ok::String> column_names,
                                     ok::Slice<ok::String> column_types) {
        auto* stmt = allocator->alloc<CreateTableStmt>();
        stmt->type = CREATE;
        stmt->token = token;
        stmt->target = Target::TABLE;
        stmt->name = name;
        stmt->column_names = column_names;
        stmt->column_types = column_types;
        return stmt;
    }

    ok::Slice<ok::String> column_names;
    ok::Slice<ok::String> column_types;
};

struct CreateUserStmt : public CreateStmt {
    static CreateUserStmt *alloc(ok::Allocator *allocator, Token token, ok::String name) {
        auto *stmt = allocator->alloc<CreateUserStmt>();
        stmt->type = CREATE;
        stmt->target = Target::USER;
        stmt->token = token;
        stmt->name = name;
        return stmt;
    }
};

struct Query {
    ok::Slice<Stmt*> stmts;
};
}; // namespace xmdb

#endif // XMDB_AST_H_
