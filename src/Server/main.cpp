#include <Core/ok.hpp>
#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>

#include <SQL/global_state.hpp>

#include <http.h>
#include <base64.h>

constexpr U16 XMDB_SERVER_PORT = 6969;
constexpr U32 XMDB_PASSWORD_HASH_LENGTH = 32;

using ConnectionId = U64;
static ConnectionId last_connection_id = 0;

struct ConnectionData {
    xmdb::DBConnection *connection;
    ok::ArenaAllocator temp_arena;
    xmdb::DBUser user;
};

static ok::ArenaAllocator shared_arena{};
static xmdb::DBPool shared_db_pool{&shared_arena};
static ok::Table<ConnectionId, ConnectionData> db_connections_table = ok::Table<ConnectionId, ConnectionData>::alloc(&shared_arena);

using ConnectionList = ok::LinkedList<xmdb::DBConnection>;
static ConnectionList connection_free_list = ConnectionList::alloc(&shared_arena);

static ConnectionId gen_connection(xmdb::DBDescriptor *db, xmdb::DBUser user) {
    xmdb::DBConnection *connection = nullptr;

    if (connection_free_list.head == nullptr) {
        connection_free_list.prepend(xmdb::DBConnection{&shared_db_pool, db});
        connection = &connection_free_list.head->value;
    } else {
        ConnectionList::Node *conn_node = connection_free_list.pop_front();
        connection = &conn_node->value;
    }

    ConnectionData connection_data = {
        .connection = connection,
        .temp_arena = {},
        .user = user,
    };

    ConnectionId connection_id = ++last_connection_id;
    db_connections_table.put(connection_id, connection_data);
    return connection_id;
}

#define DECLARE_HANDLER(name) web_http_response_status name(web_http_response_context *ctx)

DECLARE_HANDLER(connect_handler) {
    if (ctx->Request.Method != HTTP_POST) {
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    web_json_value json{};
    if (!WebHttpContextParseJsonBody(ctx, &json) || json.Type != JSON_OBJECT) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    web_json_object json_obj = json.Object;
    web_string_view username, password, db_name;

    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("username"), &username)) {
        return HTTP_STATUS_BAD_REQUEST;
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("password_hash"), &password)) {
        return HTTP_STATUS_BAD_REQUEST;
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("db_name"), &db_name)) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    // FIXME(oleh): This code *should* be uncommented, but the JS shit Base64 API uses another
    // scheme of encoding, which we currently don't support.
    // uz password_hash_length = XMDB_PASSWORD_HASH_LENGTH;
    // U8 password_hash[password_hash_length];
    // if (!WebBase64Decode(password_hash_base64, password_hash, &password_hash_length)) {
    //     return HTTP_STATUS_BAD_REQUEST;
    // }

    ok::StringView db_name_sv{(const char *)db_name.Items, db_name.Count};
    xmdb::DBDescriptor *requested_db = nullptr;

    for (xmdb::DBDescriptor *db = shared_db_pool.db_descriptors;
         db != nullptr;
         db = db->next) {
        if (db->name == db_name_sv) {
            requested_db = db;
            break;
        }
    }

    if (requested_db == nullptr) {
        return HTTP_STATUS_NOT_FOUND;
    }

    ok::StringView username_sv = {(const char *)username.Items, username.Count};
    Optional<xmdb::DBUser> found_user = Optional<xmdb::DBUser>::empty();

    for (UZ i = 0; i < requested_db->users.count; ++i) {
        xmdb::DBUser user = requested_db->users[i];
        // FIXME(oleh): Replace this password hash check against the password by a check againts
        // a computed hash.
        if (user.name == username_sv) {
            for (UZ j = 0; j < user.sha256_password_digest.get_count(); ++j) {
                if (user.sha256_password_digest[j] == 0) break;

                if (password.Items[j] != user.sha256_password_digest[j]) {
                    goto loop_end;
                }
            }

            found_user = user;
            break;
        }

    loop_end: ;
    }

    if (!found_user.has_value()) {
        return HTTP_STATUS_FORBIDDEN;
    }

    ConnectionId connection_id = gen_connection(requested_db, found_user.value);
    ctx->Content = WebArenaFormat(ctx->Arena, "%llu", connection_id);

    return HTTP_STATUS_OK;
}

DECLARE_HANDLER(run_query_handler) {
    if (ctx->Request.Method != HTTP_POST) {
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    web_json_value json;
    if (!WebHttpContextParseJsonBody(ctx, &json)) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    if (json.Type != JSON_OBJECT) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    web_json_object json_obj = json.Object;

    web_string_view connection_id_web_sv, query;
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("connection_id"), &connection_id_web_sv)) {
        return HTTP_STATUS_BAD_REQUEST;
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("query"), &query)) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    S64 connection_id;
    ok::StringView connection_id_sv{(const char *)connection_id_web_sv.Items, connection_id_web_sv.Count};
    if (!ok::parse_int64(connection_id_sv, &connection_id) || connection_id <= 0) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    Optional<ConnectionData> connection_data_opt = db_connections_table.get((U64)connection_id);
    if (!connection_data_opt.has_value()) {
        return HTTP_STATUS_NOT_FOUND;
    }

    ConnectionData connection_data = connection_data_opt.value;
    connection_data.temp_arena.reset();

    ok::StringView source_sv = {(const char *)query.Items, query.Count};
    xmdb::QueryResults query_results{};
    ok::String error{};

    bool ok = xmdb::compile_and_execute_source(&connection_data.temp_arena,
                                               connection_data.connection,
                                               source_sv,
                                               &query_results,
                                               &error);

    WebJsonBegin(ctx->Arena);
    WebJsonBeginObject();

    if (ok) {
        Optional<xmdb::DBTable *> results_table_opt = query_results.value;

        WebJsonPutKey(WEB_SV_LIT("ok"));
        WebJsonPutTrue();

        if (results_table_opt.has_value()) {
            xmdb::DBTable *results_table = results_table_opt.value;

            WebJsonPutKey(WEB_SV_LIT("column_names"));

            WebJsonBeginArray();

            for (UZ i = 0; i < results_table->columns_count; ++i) {
                ok::StringView column_name_sv = results_table->columns_names[i];
                web_string_view column_name = {
                    .Items = (u8 *)column_name_sv.data,
                    .Count = column_name_sv.count,
                };
                WebJsonPrepareArrayElement();
                WebJsonPutString(column_name);
            }

            WebJsonEndArray();

            WebJsonPutKey(WEB_SV_LIT("rows"));

            WebJsonBeginArray();

            xmdb::DBTableOutlet table_outlet = results_table->use();

            for (UZ r = 0; r < results_table->rows_count; ++r) {
                WebJsonBeginObject();

                for (UZ i = 0; i < results_table->columns_count; ++i) {
                    ok::StringView column_name_sv = results_table->columns_names[i];
                    web_string_view column_name = {
                        .Items = (u8 *)column_name_sv.data,
                        .Count = column_name_sv.count,
                    };
                    xmdb::DBValue *column_value = &table_outlet.values[i];

                    switch (column_value->type) {
                    case xmdb::SQL::TYPE_INT: {
                        xmdb::TableStream<S64> stream = column_value->u.integer;
                        Optional<S64> i = stream.next();
                        if (!i.has_value()) {
                            OK_PANIC_FMT("Expected to have %zu rows in table, but streams exhausted at %zu",
                                         results_table->rows_count,
                                         r);
                        }

                        WebJsonPutKey(column_name);
                        WebJsonPutNumber(i.value);

                        break;
                    }
                    case xmdb::SQL::TYPE_STRING: {
                        xmdb::TableStream<ok::String> stream = column_value->u.string;
                        Optional<ok::String> s = stream.next();
                        if (!s.has_value()) {
                            OK_PANIC_FMT("Expected to have %zu rows in table, but streams exhausted at %zu",
                                         results_table->rows_count,
                                         r);
                        }

                        web_string_view value = {
                            .Items = (u8 *)s.value.cstr(),
                            .Count = s.value.count(),
                        };

                        WebJsonPutKey(column_name);
                        WebJsonPutString(value);

                        break;
                    }
                    case xmdb::SQL::TYPE_BOOL: {
                        xmdb::TableStream<bool> stream = column_value->u.boolean;
                        Optional<bool> b = stream.next();
                        if (!b.has_value()) {
                            OK_PANIC_FMT("Expected to have %zu rows in table, but streams exhausted at %zu",
                                         results_table->rows_count,
                                         r);
                        }

                        WebJsonPutKey(column_name);

                        if (b.value) {
                            WebJsonPutTrue();
                        } else {
                            WebJsonPutFalse();
                        }

                        break;
                    }
                    case xmdb::SQL::TYPE_NULL: {
                        xmdb::TableStream<xmdb::Null> stream = column_value->u.null;
                        Optional<xmdb::Null> b = stream.next();
                        if (!b.has_value()) {
                            OK_PANIC_FMT("Expected to have %zu rows in table, but streams exhausted at %zu",
                                         results_table->rows_count,
                                         r);
                        }

                        WebJsonPutKey(column_name);
                        WebJsonPutNull();

                        break;
                    }
                    case xmdb::SQL::TYPE_FLOAT:
                    case xmdb::SQL::TYPE_DOUBLE:
                    case xmdb::SQL::TYPE_IMAGE: OK_TODO();
                    case xmdb::SQL::TYPE_MAX:
                    case xmdb::SQL::TYPE_TABLE: OK_UNREACHABLE();
                    }
                }

                WebJsonEndObject();
            }

            WebJsonEndArray();
        }
    } else {
        WebJsonPutKey(WEB_SV_LIT("ok"));
        WebJsonPutFalse();

        web_string_view error_message = {
            .Items = (u8 *)error.cstr(),
            .Count = error.count(),
        };
        WebJsonPutKey(WEB_SV_LIT("error_message"));
        WebJsonPutString(error_message);
    }

    WebJsonEndObject();

    web_string_view json_response = WebJsonEnd();
    ctx->Content = json_response;
    return HTTP_STATUS_OK;
}

int main() {
    xmdb::init_global_state();

    web_http_server http_server{};
    WebHttpServerInit(&http_server);

    WebHttpServerAttachHandler(&http_server, "/connect", connect_handler);
    WebHttpServerAttachHandler(&http_server, "/run-query", run_query_handler);

    printf("Starting the server on port %u\n", XMDB_SERVER_PORT);
    WebHttpServerStart(&http_server, XMDB_SERVER_PORT);

    return 0;
}
