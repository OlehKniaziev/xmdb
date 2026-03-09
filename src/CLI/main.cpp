#include <cmath>

#include <Core/ok.hpp>
#include <Core/util.hpp>
#include <Core/hash.hpp>

#include <http.h>

int main(int argc, char **argv) {
    ok::Slice<char *> args = {argv + 1, (UZ)argc - 1};
    if (args.count < 5) {
        xmdb::dief("Not enough arguments");
    }

    const char *hostname_cstr = args[0];
    const char *port_cstr = args[1];
    const char *db_name = args[2];
    const char *username = args[3];
    const char *password = args[4];

    S64 port;
    if (!ok::parse_int64(ok::StringView{port_cstr}, &port)) {
        xmdb::dief("Invalid port number '%s'", port_cstr);
    }

    xmdb::SHA256Digest password_hash = xmdb::sha256_digest(ok::StringView{password});

    web_arena arena;
    WebArenaInit(&arena, 1024 * 1024);

    xmdb::WebArenaAllocator arena_adapter{&arena};

    ok::String password_hash_hex = xmdb::to_hex_string(&arena_adapter, password_hash.bytes.slice());

    WebJsonBegin(&arena);

    WebJsonBeginObject();

    WebJsonPutKey(WEB_SV_LIT("db_name"));
    WebJsonPutString(WEB_SV_LIT(db_name));

    WebJsonPutKey(WEB_SV_LIT("username"));
    WebJsonPutString(WEB_SV_LIT(username));

    WebJsonPutKey(WEB_SV_LIT("password_hash"));
    WebJsonPutString(WEB_SV_LIT(password_hash_hex.cstr()));

    WebJsonEndObject();

    web_string_view payload = WebJsonEnd();

    web_http_request request = {
        .Method = HTTP_POST,
        .Path = WEB_SV_LIT("/connect"),
        .Version = HTTP_1_1,
        .Headers = {},
        .Body = payload,
    };

    web_string_view hostname = WEB_SV_LIT(hostname_cstr);
    web_http_response response;

    if (!WebHttpRequestSend(&arena,
                            hostname,
                            port,
                            request,
                            &response)) {
        xmdb::dief("Failed to send the HTTP connection request");
    }

    if (response.Status != HTTP_STATUS_OK) {
        xmdb::dief("Connection request to the server returned status '%s': %s",
                   WebHttpGetResponseStatusReason(response.Status),
                   response.Body);
    }

    S64 connection_id;
    ok::StringView connection_id_sv = {(const char *)response.Body.Items, response.Body.Count};
    OK_ASSERT(ok::parse_int64(connection_id_sv, &connection_id));

    uz line_buffer_count = 4096;
    char *line_buffer = (char *)malloc(sizeof(char) * line_buffer_count);

    printf("Successfully connected to the server [connection_id = %lu]\n", (U64)connection_id);

    while (true) {
        WebArenaReset(&arena);

        printf("> ");

        SZ n_read = getline(&line_buffer, &line_buffer_count, stdin);
        if (n_read == -1) {
            printf("\n");
            break;
        }

        web_string_view query_source = {.Items = (U8 *)line_buffer, .Count = (UZ)n_read - 1};

        WebJsonBegin(&arena);

        WebJsonBeginObject();

        WebJsonPutKey(WEB_SV_LIT("connection_id"));
        WebJsonPutNumber((F64)connection_id);

        WebJsonPutKey(WEB_SV_LIT("query"));
        WebJsonPutString(query_source);

        WebJsonEndObject();

        web_string_view request_body = WebJsonEnd();

        request = web_http_request {
            .Method = HTTP_POST,
            .Path = WEB_SV_LIT("/run-query"),
            .Version = HTTP_1_1,
            .Headers = {},
            .Body = request_body,
        };

        if (!WebHttpRequestSend(&arena,
                                hostname,
                                port,
                                request,
                                &response)) {
            ok::println("Failed to send a query request to the server");
            continue;
        }

        if (response.Status != HTTP_STATUS_OK) {
            const char *http_reason = WebHttpGetResponseStatusReason(response.Status);
            printf("Query request to the server failed with status '%s':" WEB_SV_FMT "\n", http_reason, WEB_SV_ARG(response.Body));
            continue;
        }

        web_json_value json;
        if (!WebJsonParse(&arena, response.Body, &json) || json.Type != JSON_OBJECT) {
            ok::println("ERROR: Invalid JSON response from the server");
            continue;
        }

        web_json_object json_obj = json.Object;

        b32 ok;
        if (!WebJsonObjectGetBool(&json_obj, WEB_SV_LIT("ok"), &ok)) {
            ok::println("ERROR: The server sent JSON is missing the 'ok' field");
            continue;
        }

        if (!ok) {
            web_string_view error_message;
            if (!WebJsonObjectGetStringView(&json_obj, WEB_SV_LIT("error_message"), &error_message)) {
                ok::println("ERROR: The server sent JSON is missing the 'error_message' field");
                continue;
            }

            printf(WEB_SV_FMT "\n", WEB_SV_ARG(error_message));
            continue;
        } else {
            web_json_array column_names;
            if (!WebJsonObjectGetArray(&json_obj, WEB_SV_LIT("column_names"), &column_names)) {
                // NOTE(oleh): This just means that our query did not yield any values.
                continue;
            }

            web_json_array rows;
            if (!WebJsonObjectGetArray(&json_obj, WEB_SV_LIT("rows"), &rows)) {
                ok::println("ERROR: The server sent JSON is missing the 'rows' field");
                continue;
            }

            for (UZ i = 0; i < rows.Count; ++i) {
                web_json_value row_json = rows.Items[i];
                if (row_json.Type != JSON_OBJECT) {
                    printf("ERROR: Row at index %zu is not an object\n", i);
                    continue;
                }

                web_json_object row = row_json.Object;

                printf("|");

                for (UZ i = 0; i < column_names.Count; ++i) {
                    web_json_value column_json = column_names.Items[i];
                    if (column_json.Type != JSON_STRING) {
                        printf("ERROR: Column name at index %zu is not a string\n", i);
                        continue;
                    }

                    web_string_view column_name = column_json.String;

                    web_json_value column_value;
                    if (!WebJsonObjectGet(&row, column_name, &column_value)) {
                        printf("ERROR: Row at index %zu is missing key '" WEB_SV_FMT "'\n",
                               i,
                               WEB_SV_ARG(column_name));
                        continue;
                    }

                    switch (column_value.Type) {
                    case JSON_NUMBER: {
                        F64 number = column_value.Number;
                        F64 integral;
                        if (modf(number, &integral) == 0.0) {
                            printf("%ld", (S64)number);
                        } else {
                            printf("%f", number);
                        }
                        break;
                    }
                    case JSON_STRING: {
                        web_string_view string = column_value.String;
                        printf("\"" WEB_SV_FMT "\"", WEB_SV_ARG(string));
                        break;
                    }
                    default: OK_TODO();
                    }

                    printf("|");
                }

                printf("\n");
            }
        }
    }

    return 0;
}
