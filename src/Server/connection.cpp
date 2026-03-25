#include "connection.hpp"

namespace xmdb::server {
static ConnectionId last_connection_id = 0;

static ok::ArenaAllocator shared_arena{};
static ok::Table<ConnectionId, ConnectionData> db_connections_table = ok::Table<ConnectionId, ConnectionData>::alloc(&shared_arena);

using ConnectionList = ok::LinkedList<xmdb::DBConnection>;
static ConnectionList connection_free_list = ConnectionList::alloc(&shared_arena);

Optional<ConnectionData &> get_connection_data(ConnectionId id) {
    return db_connections_table.get_ref(id);
}

static xmdb::DBPool *shared_db_pool;

void init_connection_state() {
    shared_db_pool = new (&shared_arena) DBPool{&shared_arena};
}


xmdb::DBPool *get_shared_db_pool() {
    return shared_db_pool;
}

ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser *user) {
    DBPool *pool = get_shared_db_pool();

    xmdb::DBConnection *connection = nullptr;

    if (connection_free_list.head == nullptr) {
        connection_free_list.prepend(xmdb::DBConnection{pool, db, user});
        connection = &connection_free_list.head->value;
    } else {
        ConnectionList::Node *conn_node = connection_free_list.pop_front();
        connection = &conn_node->value;
    }

    ConnectionData connection_data = {
        .connection = connection,
        .temp_arena = {},
        .user = user,
        .last_use_time = current_timestamp(),
    };

    ConnectionId connection_id = ++last_connection_id;
    db_connections_table.put(connection_id, connection_data);
    return connection_id;
}
} // namespace xmdb::server
