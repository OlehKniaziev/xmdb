#pragma once

#include <SQL/ir.hpp>

#include "DBPool.hpp"
#include "ok.hpp"

namespace xmdb {
struct QueryResults {
    bool ok;
    // UZ rows_affected;
    Optional<DBTable *> value;
};

struct DBConnection {
    explicit DBConnection(DBPool *, DBDescriptor *);

    void execute(SQL::TypedCompiledQuery *query, QueryResults *results);

    void close();

    DBPool *db_pool;
    DBDescriptor *db;
};
};
