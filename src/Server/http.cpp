#include <cmath>

#include <Core/Core.hpp>
#include <Core/DBDescriptor.hpp>
#include <Core/ok.hpp>

#include <http.h>
#include <log.h>

#include "connection.hpp"
#include "http.hpp"
#include "obj.hpp"

namespace xmdb::server {
#define DECLARE_HANDLER(name) web_http_response_status name(web_http_response_context *ctx)

#define FAIL(status, reason) do { \
    WebHttpResponseWrite(ctx, WEB_SV_LIT(reason)); \
    return HTTP_STATUS_##status; \
    } while (false)

DECLARE_HANDLER(connect_handler) {
    if (ctx->Request.Method != HTTP_POST) {
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    web_json_value json{};
    if (!WebHttpContextParseJsonBody(ctx, &json) || json.Type != JSON_OBJECT) {
        FAIL(BAD_REQUEST, "request is not a valid JSON object");
    }

    web_json_object json_obj = json.Object;
    web_string_view username, password_hash_hex, db_name;

    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("username"), &username)) {
        FAIL(BAD_REQUEST, "'username' field not present");
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("password_hash"), &password_hash_hex)) {
        FAIL(BAD_REQUEST, "'password_hash' field not present");
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("db_name"), &db_name)) {
        FAIL(BAD_REQUEST, "'db_name' field not present");
    }

    WebArenaAllocator ctx_arena{&ctx->Arena};

    ok::StringView password_hash_hex_sv = {(const char *) password_hash_hex.Items, password_hash_hex.Count};

    ok::Optional<ok::Slice<U8>> password_hash_opt = xmdb::from_hex_string(&ctx_arena, password_hash_hex_sv);
    if (!password_hash_opt.has_value()) {
        FAIL(BAD_REQUEST, "'password_hash' field is not a valid hex string");
    }

    ok::Slice<U8> password_hash = password_hash_opt.get();

    ok::StringView db_name_sv{(const char *) db_name.Items, db_name.Count};
    xmdb::DBDescriptor *requested_db = nullptr;

    for (xmdb::DBDescriptor *db = shared_db_pool.db_descriptors; db != nullptr; db = db->next) {
        if (db->name == db_name_sv) {
            requested_db = db;
            break;
        }
    }

    if (requested_db == nullptr) {
        FAIL(NOT_FOUND, "requested database not found");
    }

    ok::StringView username_sv = {(const char *) username.Items, username.Count};

    Optional<DBUser *> user_opt = requested_db->find_user(username_sv);

    if (!user_opt.has_value()) {
        FAIL(NOT_FOUND, "requested user not found");
    }

    DBUser *user = user_opt.value;

    if (password_hash.count != user->sha256_password_digest.bytes.get_count()) {
        FAIL(BAD_REQUEST, "invalid password digest size");
    }

    // FIXME(oleh): Replace this password hash check against the password by a check againts
    // a computed hash.
    for (UZ j = 0; j < user->sha256_password_digest.bytes.get_count(); ++j) {
        if (password_hash.items[j] != user->sha256_password_digest.bytes[j]) {
            FAIL(BAD_REQUEST, "provided password digest is incorrect");
        }
    }

    ConnectionId connection_id = gen_connection(requested_db, user);
    ctx->Content = WebArenaFormat(&ctx->Arena, "%llu", connection_id);

    return HTTP_STATUS_OK;
}

DECLARE_HANDLER(run_query_handler) {
    if (ctx->Request.Method != HTTP_POST) {
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    web_json_value json;
    if (!WebHttpContextParseJsonBody(ctx, &json) || json.Type != JSON_OBJECT) {
        FAIL(BAD_REQUEST, "request body is not a valid JSON object");
    }

    web_json_object json_obj = json.Object;

    F64 connection_id_f64;
    web_string_view query;
    if (!WebJsonObjectGetNumber(&json_obj, WEB_SV_LIT("connection_id"), &connection_id_f64)) {
        FAIL(BAD_REQUEST, "'connection_id' field not present in the request body");
    }
    if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("query"), &query)) {
        FAIL(BAD_REQUEST, "'query' field not present in the request body");
    }

    F64 integral;
    ConnectionId connection_id;
    if (connection_id_f64 < 0.0 || modf(connection_id_f64, &integral) != 0.0) {
        FAIL(BAD_REQUEST, "'connection_id' field should be integral");
    }

    connection_id = (ConnectionId) connection_id_f64;

    Optional<ConnectionData> connection_data_opt = get_connection_data(connection_id);
    if (!connection_data_opt.has_value()) {
        FAIL(BAD_REQUEST, "connection with requested id was not found");
    }

    ConnectionData connection_data = connection_data_opt.value;
    connection_data.temp_arena.reset();

    ok::StringView source_sv = {(const char *) query.Items, query.Count};
    xmdb::QueryResults query_results{};
    ok::String error{};

    bool ok = xmdb::compile_and_execute_source(&connection_data.temp_arena, connection_data.connection, source_sv,
                                               &query_results, &error);

    WebJsonBegin(&ctx->Arena);
    WebJsonBeginObject();

    if (ok) {
        Optional<xmdb::DBTable *> results_table_opt = query_results.value;

        WebJsonPutKey(WEB_SV_LIT("ok"));
        WebJsonPutTrue();

        if (results_table_opt.has_value()) {
            xmdb::DBTable *results_table = results_table_opt.get();

            WebJsonPutKey(WEB_SV_LIT("column_names"));

            WebJsonBeginArray();

            for (UZ i = 0; i < results_table->columns_count(); ++i) {
                ok::StringView column_name_sv = results_table->columns_names()[i];
                web_string_view column_name = {
                    .Items = (u8 *) column_name_sv.data,
                    .Count = column_name_sv.count,
                };
                WebJsonPrepareArrayElement();
                WebJsonPutString(column_name);
            }

            WebJsonEndArray();

            WebJsonPutKey(WEB_SV_LIT("column_types"));

            WebJsonBeginArray();

            for (UZ i = 0; i < results_table->columns_count(); ++i) {
                xmdb::SQL::ColumnType column_type = results_table->columns_types()[i];
                const char *column_type_str = xmdb::SQL::column_type_to_string(column_type);
                web_string_view column_type_sv = {
                    .Items = (u8 *) column_type_str,
                    .Count = strlen(column_type_str),
                };
                WebJsonPrepareArrayElement();
                WebJsonPutString(column_type_sv);
            }

            WebJsonEndArray();

            WebJsonPutKey(WEB_SV_LIT("rows"));

            WebJsonBeginArray();

            xmdb::DBTableOutlet table_outlet{results_table};
            xmdb::DBTableStream *column_streams_ptr = connection_data.temp_arena.alloc<xmdb::DBTableStream>(results_table->columns_count());
            ok::Slice<xmdb::DBTableStream> column_streams = {column_streams_ptr, results_table->columns_count()};
            for (UZ i = 0; i < results_table->columns_count(); ++i) {
                column_streams[i] = table_outlet.column_stream(&connection_data.temp_arena, i);
            }

            for (UZ r = 0; r < results_table->rows_count(); ++r) {
                WebJsonPrepareArrayElement();

                WebJsonBeginObject();

                for (UZ i = 0; i < results_table->columns_count(); ++i) {
                    ok::StringView column_name_sv = results_table->columns_names()[i];
                    web_string_view column_name = {
                        .Items = (u8 *) column_name_sv.data,
                        .Count = column_name_sv.count,
                    };

                    xmdb::DBTableStream *column_stream = &column_streams[i];
                    xmdb::Value value = column_stream->next().get();

                    WebJsonPutKey(column_name);

                    switch (value.type()) {
                    case Value::Type::INT: {
                        S64 n = value.as_int();
                        WebJsonPutNumber(n);
                        break;
                    }
                    case Value::Type::STRING: {
                        FixedString s = value.as_string();
                        web_string_view value = {
                            .Items = (u8 *) s.items,
                            .Count = s.count,
                        };

                        WebJsonPutString(value);

                        break;
                    }
                    case Value::Type::BOOL: {
                        bool b = value.as_bool();

                        if (b) {
                            WebJsonPutTrue();
                        } else {
                            WebJsonPutFalse();
                        }

                        break;
                    }
                    case Value::Type::IMAGE_CHUNK: {
                        ImageChunk *chunk = value.as_chunk();

                        WebJsonBeginObject();

                        WebJsonPutKey(WEB_SV_LIT("x"));
                        WebJsonPutNumber(chunk->x);

                        WebJsonPutKey(WEB_SV_LIT("y"));
                        WebJsonPutNumber(chunk->y);

                        WebJsonPutKey(WEB_SV_LIT("width"));
                        WebJsonPutNumber(chunk->width);

                        WebJsonPutKey(WEB_SV_LIT("height"));
                        WebJsonPutNumber(chunk->height);

                        ok::String data_hex = xmdb::to_hex_string(&connection_data.temp_arena, chunk->data);

                        WebJsonPutKey(WEB_SV_LIT("data"));
                        WebJsonPutString((web_string_view){.Items = (u8 *) data_hex.cstr(), .Count = data_hex.count()});

                        WebJsonEndObject();

                        break;
                    }
                    case Value::Type::BIG_STRING: {
                        OK_UNREACHABLE();
                    }
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
            .Items = (u8 *) error.cstr(),
            .Count = error.count(),
        };
        WebJsonPutKey(WEB_SV_LIT("error_message"));
        WebJsonPutString(error_message);
    }

    WebJsonEndObject();

    web_string_view json_response = WebJsonEnd();
    ctx->Content = json_response;

    WebHttpContextAddHeader(ctx, WEB_SV_LIT("Content-Type"), WEB_SV_LIT("application/json"));
    return HTTP_STATUS_OK;
}

DECLARE_HANDLER(get_db_objects_handler) {
    if (ctx->Request.Method != HTTP_POST) {
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    web_json_value json;
    if (!WebHttpContextParseJsonBody(ctx, &json) || json.Type != JSON_OBJECT) {
        FAIL(BAD_REQUEST, "request body is not a valid JSON object");
    }

    web_json_object json_obj = json.Object;

    ConnectionId connection_id;
    if (!WebJsonObjectGetU64(&json_obj, WEB_SV_LIT("connection_id"), &connection_id)) {
        FAIL(BAD_REQUEST, "'connection_id' field not present in the request body");
    }

    Optional<ConnectionData> connection_data_opt = get_connection_data(connection_id);
    if (!connection_data_opt.has_value()) {
        FAIL(BAD_REQUEST, "connection with requested id was not found");
    }

    ConnectionData connection_data = connection_data_opt.get();
    connection_data.temp_arena.reset();

    DBDescriptor *db = connection_data.connection->db;
    ok::Slice<TableObject> table_objects = get_db_table_objects(&connection_data.temp_arena, db);

    WebJsonBegin(&ctx->Arena);
    WebJsonBeginObject();

    WebJsonPutKey(WEB_SV_LIT("ok"));
    WebJsonPutTrue();

    WebJsonPutKey(WEB_SV_LIT("tables"));
    WebJsonBeginArray();

    for (UZ table_idx = 0; table_idx < table_objects.count; ++table_idx) {
        TableObject &table = table_objects.items[table_idx];

        WebJsonPrepareArrayElement();
        WebJsonBeginObject();

        WebJsonPutKey(WEB_SV_LIT("name"));
        WebJsonPutString((web_string_view){.Items = (u8 *) table.name.data, .Count = table.name.count});

        WebJsonPutKey(WEB_SV_LIT("column_names"));
        WebJsonBeginArray();
        for (UZ col_idx = 0; col_idx < table.column_names.count; ++col_idx) {
            ok::StringView col_name = table.column_names.items[col_idx];
            WebJsonPrepareArrayElement();
            WebJsonPutString((web_string_view){.Items = (u8 *) col_name.data, .Count = col_name.count});
        }
        WebJsonEndArray();

        WebJsonPutKey(WEB_SV_LIT("column_types"));
        WebJsonBeginArray();
        for (UZ col_idx = 0; col_idx < table.column_types.count; ++col_idx) {
            xmdb::SQL::ColumnType col_type = table.column_types.items[col_idx];
            const char *col_type_str = xmdb::SQL::column_type_to_string(col_type);
            WebJsonPrepareArrayElement();
            WebJsonPutString((web_string_view){.Items = (u8 *) col_type_str, .Count = strlen(col_type_str)});
        }
        WebJsonEndArray();

        WebJsonEndObject();
    }

    WebJsonEndArray();

    WebJsonEndObject();

    web_string_view json_response = WebJsonEnd();
    ctx->Content = json_response;

    WebHttpContextAddHeader(ctx, WEB_SV_LIT("Content-Type"), WEB_SV_LIT("application/json"));
    return HTTP_STATUS_OK;
}

void run_http_server(U16 port) {
    WebLogSetDestination(stdout);

    web_http_server server{};
    web_http_server_config config{
        // TODO(oleh): Make this configurable for user.
        .NumThreads = 1,
        .UseHttps = false,
        .HttpsProvider = nullptr,
    };
    WebHttpServerInit(&server, &config);

    WebHttpServerAttachHandler(&server, "/connect", connect_handler);
    WebHttpServerAttachHandler(&server, "/run-query", run_query_handler);
    WebHttpServerAttachHandler(&server, "/get-db-objects", get_db_objects_handler);

    WebHttpServerStart(&server, port);
}
} // namespace xmdb::server
