#pragma once

#include <Core/DBConnection.hpp>

namespace xmdb::server
{
using ConnectionId = U64;

using Timestamp = time_t;

static inline Timestamp current_timestamp()
{
    return time(NULL);
}

struct ConnectionData
{
    ConnectionData *next;
    xmdb::DBConnection *connection;
    ok::ArenaAllocator temp_arena;
    xmdb::DBUser *user;
    Timestamp last_use_time;
};

void init_connection_state();

xmdb::DBPool *get_shared_db_pool();

ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser *user);
void free_connection(ConnectionId);
Optional<ConnectionData *> get_connection_data(ConnectionId);
} // namespace xmdb::server
