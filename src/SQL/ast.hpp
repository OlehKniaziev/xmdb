#ifndef XMDB_AST_H_
#define XMDB_AST_H_

#include <Core/ok.hpp>

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
    };

    Type type;
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
    };

    static Expr* true_literal;
    static Expr* false_literal;
    static Expr* null_literal;

    Type type;
};

struct ExprIdentifier : public Expr {
    static ExprIdentifier* alloc(ok::Allocator* allocator, ok::StringView value) {
        auto* expr = allocator->alloc<ExprIdentifier>();
        expr->type = IDENT;
        expr->value = value.to_string(allocator);
        return expr;
    }

    ok::String value;
};

struct ExprInteger : public Expr {
    static ExprInteger* alloc(ok::Allocator* allocator, int64_t value) {
        auto* expr = allocator->alloc<ExprInteger>();
        expr->type = INTEGER_LIT;
        expr->value = value;
        return expr;
    }

    int64_t value;
};

struct ExprString : public Expr {
    static ExprString* alloc(ok::Allocator* allocator, ok::String value) {
        auto* expr = allocator->alloc<ExprString>();
        expr->type = STRING_LIT;
        expr->value = value;
        return expr;
    }

    ok::String value;
};

struct ExprBinaryOp : public Expr {
    enum class Kind : uint8_t {
        EQ,
        GT,
        LT,
    };

    static ExprBinaryOp* alloc(ok::Allocator* allocator, Kind kind, Expr* left, Expr* right) {
        auto* expr = allocator->alloc<ExprBinaryOp>();
        expr->type = BINARY_OP;
        expr->kind = kind;
        expr->left = left;
        expr->right = right;
        return expr;
    }

    Kind kind;
    Expr* left;
    Expr* right;
};

struct SelectStmt : public Stmt {
    static SelectStmt* alloc(ok::Allocator* allocator, ok::Slice<Expr*> exprs, Expr* table) {
        auto* stmt = allocator->alloc<SelectStmt>();
        stmt->type = SELECT;
        stmt->exprs = exprs;
        stmt->table = table;
        return stmt;
    }

    ok::Slice<Expr*> exprs;
    Expr* table;
};

struct UseStmt : public Stmt {
    static UseStmt* alloc(ok::Allocator* allocator, ok::StringView database) {
        auto* stmt = allocator->alloc<UseStmt>();
        stmt->type = USE;
        stmt->database = database.to_string(allocator);
        return stmt;
    }

    ok::String database;
};

struct InsertStmt : public Stmt {
    static InsertStmt* alloc(ok::Allocator* allocator, Expr* table, ok::Slice<ok::String> columns,
                                ok::Slice<Expr*> values, ok::Slice<uint32_t> values_counts) {
        auto* stmt = allocator->alloc<InsertStmt>();
        stmt->type = INSERT;
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
    static UpdateStmt* alloc(ok::Allocator* allocator, Expr* table, ok::Slice<ok::String> columns,
                                ok::Slice<Expr*> values, ok::Optional<Expr*> filter) {
        auto* stmt = allocator->alloc<UpdateStmt>();
        stmt->type = UPDATE;
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
    static DeleteStmt* alloc(ok::Allocator* allocator, Expr* table, ok::Optional<Expr*> filter) {
        auto* stmt = allocator->alloc<DeleteStmt>();
        stmt->type = DELETE;
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

    static DropStmt* alloc(ok::Allocator* allocator, Target target, ok::String name) {
        auto* stmt = allocator->alloc<DropStmt>();
        stmt->type = DROP;
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
    };

    Target target;
    ok::String name;
};

struct CreateDatabaseStmt : public CreateStmt {
    static CreateDatabaseStmt* alloc(ok::Allocator* allocator, ok::String name) {
        auto* stmt = allocator->alloc<CreateDatabaseStmt>();
        stmt->type = CREATE;
        stmt->target = Target::DATABASE;
        stmt->name = name;
        return stmt;
    }
};

struct CreateTableStmt : public CreateStmt {
    static CreateTableStmt* alloc(ok::Allocator* allocator, ok::String name, ok::Slice<ok::String> column_names,
                                     ok::Slice<ok::String> column_types) {
        auto* stmt = allocator->alloc<CreateTableStmt>();
        stmt->type = CREATE;
        stmt->target = Target::TABLE;
        stmt->name = name;
        stmt->column_names = column_names;
        stmt->column_types = column_types;
        return stmt;
    }

    ok::Slice<ok::String> column_names;
    ok::Slice<ok::String> column_types;
};
}; // namespace xmdb

#endif // XMDB_AST_H_
