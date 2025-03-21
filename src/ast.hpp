#ifndef XMDB_AST_H_
#define XMDB_AST_H_

#include "ok.hpp"

namespace xmdb {
struct SQLStmt {
    enum Type {
        SELECT,
        USE,
        INSERT,
        UPDATE,
        DELETE,
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

struct SQLUseStmt : public SQLStmt {
    static SQLUseStmt* alloc(ok::Allocator* allocator, ok::StringView database) {
        auto* stmt = allocator->alloc<SQLUseStmt>();
        stmt->type = USE;
        stmt->database = database.to_string(allocator);
        return stmt;
    }

    ok::String database;
};

struct SQLInsertStmt : public SQLStmt {
    static SQLInsertStmt* alloc(ok::Allocator* allocator, SQLExpr* table, ok::Slice<ok::String> columns,
                                ok::Slice<SQLExpr*> values, ok::Slice<uint32_t> values_counts) {
        auto* stmt = allocator->alloc<SQLInsertStmt>();
        stmt->type = INSERT;
        stmt->table = table;
        stmt->columns = columns;
        stmt->values = values;
        stmt->values_counts = values_counts;
        return stmt;
    }

    SQLExpr* table;
    ok::Slice<ok::String> columns;
    ok::Slice<SQLExpr*> values;
    ok::Slice<uint32_t> values_counts;
};

struct SQLUpdateStmt : public SQLStmt {
    static SQLUpdateStmt* alloc(ok::Allocator* allocator, SQLExpr* table, ok::Slice<ok::String> columns,
                                ok::Slice<SQLExpr*> values, ok::Optional<SQLExpr*> filter) {
        auto* stmt = allocator->alloc<SQLUpdateStmt>();
        stmt->type = UPDATE;
        stmt->table = table;
        stmt->columns = columns;
        stmt->values = values;
        stmt->filter = filter;
        return stmt;
    }

    SQLExpr* table;
    ok::Slice<ok::String> columns;
    ok::Slice<SQLExpr*> values;
    ok::Optional<SQLExpr*> filter;
};

struct SQLDeleteStmt : public SQLStmt {
    static SQLDeleteStmt* alloc(ok::Allocator* allocator, SQLExpr* table, ok::Optional<SQLExpr*> filter) {
        auto* stmt = allocator->alloc<SQLDeleteStmt>();
        stmt->type = DELETE;
        stmt->table = table;
        stmt->filter = filter;
        return stmt;
    }

    SQLExpr* table;
    ok::Optional<SQLExpr*> filter;
};
}; // namespace xmdb

#endif // XMDB_AST_H_
