#pragma once

#include <SQL/ir.hpp>

#include "DBPool.hpp"
#include "ok.hpp"

namespace xmdb {
struct DBConnection {
    explicit DBConnection(DBPool *);

    void execute(SQL::CompiledQuery *query);

    void close();

    DBPool *db_pool;
};
};
