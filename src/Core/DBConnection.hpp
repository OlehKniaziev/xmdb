#pragma once

#include <SQL/ir.hpp>

#include "DBPool.hpp"
#include "ok.hpp"

namespace xmdb {
struct QueryResults {
    Optional<ErrorWithSourceLocation> error;
    Optional<DBTable *> value;
};

struct DBConnection {
    explicit DBConnection(DBPool *, DBDescriptor *, DBUser *);

    void execute(SQL::TypedCompiledQuery *query, QueryResults *results);

    void close();

    DBPool *db_pool;
    DBUser *user;
    DBDescriptor *db;
    SQL::IrContext ir_ctx;
};
}
