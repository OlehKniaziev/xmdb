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
        DROP,
        CREATE,
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

struct SQLDropStmt : public SQLStmt {
    enum class Target : uint8_t {
        TABLE,
        DATABASE,
    };

    static SQLDropStmt* alloc(ok::Allocator* allocator, Target target, ok::String name) {
        auto* stmt = allocator->alloc<SQLDropStmt>();
        stmt->type = DROP;
        stmt->target = target;
        stmt->name = name;
        return stmt;
    }

    Target target;
    ok::String name;
};

struct SQLCreateStmt : public SQLStmt {
    enum class Target : uint8_t {
        DATABASE,
        TABLE,
    };

    Target target;
    ok::String name;
};

struct SQLCreateDatabaseStmt : public SQLCreateStmt {
    static SQLCreateDatabaseStmt* alloc(ok::Allocator* allocator, ok::String name) {
        auto* stmt = allocator->alloc<SQLCreateDatabaseStmt>();
        stmt->type = CREATE;
        stmt->target = Target::DATABASE;
        stmt->name = name;
        return stmt;
    }
};

struct SQLCreateTableStmt : public SQLCreateStmt {
    static SQLCreateTableStmt* alloc(ok::Allocator* allocator, ok::String name, ok::Slice<ok::String> column_names,
                                     ok::Slice<ok::String> column_types) {
        auto* stmt = allocator->alloc<SQLCreateTableStmt>();
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
