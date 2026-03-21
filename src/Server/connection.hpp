#pragma once

#include <Core/DBConnection.hpp>

namespace xmdb::server {
using ConnectionId = U64;

struct ConnectionData {
    xmdb::DBConnection *connection;
    ok::ArenaAllocator temp_arena;
    xmdb::DBUser *user;
};

void init_connection_state();

xmdb::DBPool *get_shared_db_pool();

ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser *user);
Optional<ConnectionData> get_connection_data(ConnectionId);
} // namespace xmdb::server
