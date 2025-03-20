#ifndef XMDB_AST_H_
#define XMDB_AST_H_

#include "ok.hpp"

namespace xmdb {
struct SQLStmt {
    enum Type {
        SELECT,
    };

    Type type;
};

struct SQLExpr {
    enum Type {
        IDENT,
    };

    Type type;
};

struct SQLExprIdentifier : public SQLExpr {
    static SQLExprIdentifier* alloc(ok::Allocator* allocator, ok::StringView value) {
        auto* expr = allocator->alloc<SQLExprIdentifier>();
        expr->type = IDENT;
        expr->value = value.to_string(allocator);
        return expr;
    }

    ok::String value;
};

struct SQLSelectStmt : public SQLStmt {
    static SQLSelectStmt* alloc(ok::Allocator* allocator, ok::Slice<SQLExpr*> exprs, SQLExpr* table) {
        auto* stmt = allocator->alloc<SQLSelectStmt>();
        stmt->type = SELECT;
        stmt->exprs = exprs;
        stmt->table = table;
        return stmt;
    }

    ok::Slice<SQLExpr*> exprs;
    SQLExpr* table;
};
};

#endif // XMDB_AST_H_
