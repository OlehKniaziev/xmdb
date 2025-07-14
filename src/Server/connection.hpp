#pragma once

#include <Core/DBConnection.hpp>

namespace xmdb::server {
using ConnectionId = U64;

extern xmdb::DBPool shared_db_pool;

struct ConnectionData {
    xmdb::DBConnection *connection;
    ok::ArenaAllocator temp_arena;
    xmdb::DBUser user;
};

ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser user);
Optional<ConnectionData> get_connection_data(ConnectionId);
} // namespace xmdb::server
