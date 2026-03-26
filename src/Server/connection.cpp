#include "connection.hpp"

namespace xmdb::server {
static ConnectionId last_connection_id = 0;

static xmdb::MallocAllocator shared_alloc{};
static ok::Table<ConnectionId, ConnectionData *> db_connections_table = ok::Table<ConnectionId, ConnectionData *>::alloc(&shared_alloc);

static ConnectionData *connection_free_list;

Optional<ConnectionData *> get_connection_data(ConnectionId id) {
    return db_connections_table.get(id);
}

static xmdb::DBPool *shared_db_pool;

void init_connection_state() {
    shared_db_pool = new (&shared_alloc) DBPool{&shared_alloc};
}


xmdb::DBPool *get_shared_db_pool() {
    return shared_db_pool;
}

static void free_connection_data(ConnectionData *data) {
    data->temp_arena.free();
    shared_alloc.dealloc(data->connection, 1);
    data->next = connection_free_list;
    connection_free_list = data;
}

static constexpr S64 SECOND = 1;
static constexpr S64 MINUTE = 60 * SECOND;
static constexpr S64 TIMEOUT_DURATION = 15 * MINUTE;

ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser *user) {
    Timestamp now = current_timestamp();

    OK_TABLE_FOREACH(db_connections_table, conn_id, conn_data, {
        (void) conn_id;
        if (now - conn_data->last_use_time >= TIMEOUT_DURATION) {
            free_connection_data(conn_data);
        }
    });

    DBPool *pool = get_shared_db_pool();

    ConnectionData *connection_data = nullptr;
    // TODO(oleh): These should also be pooled.
    DBConnection *connection = new (&shared_alloc) xmdb::DBConnection{pool, db, user};

    if (connection_free_list == nullptr) {
        connection_data = new (&shared_alloc) ConnectionData{
            .next = nullptr,
            .connection = connection,
            .temp_arena = {},
            .user = user,
            .last_use_time = now,
        };
    } else {
        connection_data = connection_free_list;
        connection_free_list = connection_data->next;
        // shared_alloc.dealloc(data->connection, 1);

        connection_data->connection = connection;
        connection_data->temp_arena = {};
        connection_data->last_use_time = now;
        connection_data->user = user;
    }

    ConnectionId connection_id = ++last_connection_id;
    db_connections_table.put(connection_id, connection_data);
    return connection_id;
}
} // namespace xmdb::server
