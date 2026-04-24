#include <cmath>

#include <ArgParser/ArgParser.hpp>

#include <Core/Logger.hpp>
#include <Core/Result.hpp>
#include <Core/hash.hpp>
#include <Core/image.hpp>
#include <Core/ok.hpp>
#include <Core/util.hpp>
#include <SQL/ir.hpp>

#include <http.h>

static xmdb::Result<xmdb::ImageChunk, ok::String>
parse_image_chunk_from_json_object(ok::Allocator *allocator,
                                   web_json_object object)
{
    U32 x, y, width, height;
    if (!WebJsonObjectGetU32(&object, WEB_SV_LIT("x"), &x))
    {
        return ok::String::format(
                allocator,
                "the image chunk JSON object is missing the 'x' field");
    }
    if (!WebJsonObjectGetU32(&object, WEB_SV_LIT("y"), &y))
    {
        return ok::String::format(
                allocator,
                "the image chunk JSON object is missing the 'y' field");
    }
    if (!WebJsonObjectGetU32(&object, WEB_SV_LIT("width"), &width))
    {
        return ok::String::format(
                allocator,
                "the image chunk JSON object is missing the 'width' field");
    }
    if (!WebJsonObjectGetU32(&object, WEB_SV_LIT("height"), &height))
    {
        return ok::String::format(
                allocator,
                "the image chunk JSON object is missing the 'height' field");
    }

    web_string_view data_web_sv;
    if (!WebJsonObjectGetStringView(&object, WEB_SV_LIT("data"), &data_web_sv))
    {
        return ok::String::format(
                allocator,
                "the image chunk JSON object is missing the 'data' field");
    }

    ok::StringView data_sv = {(const char *) data_web_sv.Items,
                              data_web_sv.Count};
    ok::Optional<ok::Slice<U8>> data_opt =
            xmdb::from_hex_string(allocator, data_sv);
    if (!data_opt.has_value())
    {
        return ok::String::format(
                allocator,
                "the image chunk's 'data' field is not a valid hex string");
    }

    ok::Slice<U8> data = data_opt.get();

    XMDB_FIXME("No way to currently obtain the pixel format of the image, so "
               "setting chunk's pixel format to RGB");

    return xmdb::ImageChunk{
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .data = data,
            .pixel_format = xmdb::PixelFormat::RGB,
    };
}

int main(int argc, char **argv)
{
    xmdb::argparser::ArgParser parser{argc, argv};

    const char *hostname, *db, *username, *password;
    S64 port;
    bool debug;

    parser.string("hostname", &hostname,
                  "The hostname or IP address on which the server is running");
    parser.string("db", &db, "Database name to use");
    parser.string("user", &username, "User name to connect as");
    parser.string("password", &password, "User's password");
    parser.integer("port", &port,
                   "The port on which to establish the connection", 6969);
    parser.boolean("debug", &debug, "Enable debug logging");

    if (argc == 1)
    {
        parser.help();
        return 1;
    }

    if (!parser.parse())
    {
        ok::StringView error_message = parser.error_message();
        parser.help();
        printf("\nFailed to parse command line flags: " OK_SV_FMT "\n",
               OK_SV_ARG(error_message));
        return 1;
    }

    parser.dealloc();

    if (debug) xmdb::set_log_level(xmdb::LogLevel::DEBUG);

    xmdb::SHA256Digest password_hash =
            xmdb::sha256_digest(ok::StringView{password});

    web_arena arena;
    WebArenaInit(&arena, 1024 * 1024);

    xmdb::WebArenaAllocator arena_adapter{&arena};

    ok::String password_hash_hex =
            xmdb::to_hex_string(&arena_adapter, password_hash.bytes.slice());

    WebJsonBegin(&arena);

    WebJsonBeginObject();

    WebJsonPutKey(WEB_SV_LIT("db_name"));
    WebJsonPutString(WEB_SV_LIT(db));

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

    web_string_view hostname_web_sv = WEB_SV_LIT(hostname);
    web_http_response response;

    if (!WebHttpRequestSend(&arena, hostname_web_sv, port, request, &response))
    {
        xmdb::dief("Failed to send the HTTP connection request");
    }

    if (response.Status != HTTP_STATUS_OK)
    {
        xmdb::dief("Connection request to the server returned status "
                   "'%s': " WEB_SV_FMT,
                   WebHttpGetResponseStatusReason(response.Status),
                   WEB_SV_ARG(response.Body));
    }

    S64 connection_id;
    ok::StringView connection_id_sv = {(const char *) response.Body.Items,
                                       response.Body.Count};
    OK_ASSERT(ok::parse_int64(connection_id_sv, &connection_id));

    uz line_buffer_count = 4096;
    char *line_buffer = (char *) malloc(sizeof(char) * line_buffer_count);

    printf("Successfully connected to the server [connection_id = %lu]\n",
           (U64) connection_id);

    while (true)
    {
    main_loop:
        WebArenaReset(&arena);

        printf("> ");

        SZ n_read = getline(&line_buffer, &line_buffer_count, stdin);
        if (n_read == -1)
        {
            printf("\n");
            break;
        }

        web_string_view query_source = {.Items = (U8 *) line_buffer,
                                        .Count = (UZ) n_read - 1};

        WebJsonBegin(&arena);

        WebJsonBeginObject();

        WebJsonPutKey(WEB_SV_LIT("connection_id"));
        WebJsonPutNumber((F64) connection_id);

        WebJsonPutKey(WEB_SV_LIT("query"));
        WebJsonPutString(query_source);

        WebJsonEndObject();

        web_string_view request_body = WebJsonEnd();

        request = web_http_request{
                .Method = HTTP_POST,
                .Path = WEB_SV_LIT("/run-query"),
                .Version = HTTP_1_1,
                .Headers = {},
                .Body = request_body,
        };

        if (!WebHttpRequestSend(&arena, hostname_web_sv, port, request,
                                &response))
        {
            ok::println("Failed to send a query request to the server");
            continue;
        }

        xmdb::log::debug("/run-query endpoint response body: " WEB_SV_FMT,
                         WEB_SV_ARG(response.Body));

        if (response.Status != HTTP_STATUS_OK)
        {
            const char *http_reason =
                    WebHttpGetResponseStatusReason(response.Status);
            printf("Query request to the server failed with status "
                   "'%s': " WEB_SV_FMT "\n",
                   http_reason, WEB_SV_ARG(response.Body));
            continue;
        }

        web_json_value json;
        if (!WebJsonParse(&arena, response.Body, &json) ||
            json.Type != JSON_OBJECT)
        {
            printf("ERROR: invalid JSON response from the server: " WEB_SV_FMT
                   "\n",
                   WEB_SV_ARG(response.Body));
            continue;
        }

        web_json_object json_obj = json.Object;

        b32 ok;
        if (!WebJsonObjectGetBool(&json_obj, WEB_SV_LIT("ok"), &ok))
        {
            ok::println(
                    "ERROR: The server sent JSON is missing the 'ok' field");
            continue;
        }

        if (!ok)
        {
            web_string_view error_message;
            if (!WebJsonObjectGetStringView(
                        &json_obj, WEB_SV_LIT("error_message"), &error_message))
            {
                ok::println("ERROR: The server sent JSON is missing the "
                            "'error_message' field");
                continue;
            }

            printf(WEB_SV_FMT "\n", WEB_SV_ARG(error_message));
            continue;
        }
        else
        {
            web_json_array column_names;
            if (!WebJsonObjectGetArray(&json_obj, WEB_SV_LIT("column_names"),
                                       &column_names))
            {
                // NOTE(oleh): This just means that our query did not yield any
                // values.
                continue;
            }

            web_json_array json_column_types;
            if (!WebJsonObjectGetArray(&json_obj, WEB_SV_LIT("column_types"),
                                       &json_column_types))
            {
                ok::println("ERROR: The server sent JSON is missing the "
                            "'column_types' field");
                continue;
            }

            xmdb::SQL::ColumnType *column_types_ptr =
                    arena_adapter.alloc<xmdb::SQL::ColumnType>(
                            json_column_types.Count);
            ok::Slice<xmdb::SQL::ColumnType> column_types = {
                    column_types_ptr, json_column_types.Count};

            for (UZ i = 0; i < json_column_types.Count; ++i)
            {
                web_json_value column_type_val = json_column_types.Items[i];
                if (column_type_val.Type != JSON_STRING)
                {
                    printf("ERROR: 'column_types' array member at position %zu "
                           "is not of type 'string'\n",
                           i);
                    goto main_loop;
                }

                web_string_view column_type_web_sv = column_type_val.String;
                ok::StringView column_type_sv = {
                        (const char *) column_type_web_sv.Items,
                        column_type_web_sv.Count};

                ok::Optional<xmdb::SQL::ColumnType> column_type_opt =
                        xmdb::SQL::parse_column_type(column_type_sv);
                if (!column_type_opt.has_value())
                {
                    printf("ERROR: could not parse the column type '" OK_SV_FMT
                           "' at position %zu as a valid column type\n",
                           OK_SV_ARG(column_type_sv), i);
                    goto main_loop;
                }

                xmdb::SQL::ColumnType column_type = column_type_opt.get();
                column_types[i] = column_type;
            }

            web_json_array rows;
            if (!WebJsonObjectGetArray(&json_obj, WEB_SV_LIT("rows"), &rows))
            {
                ok::println("ERROR: The server sent JSON is missing the 'rows' "
                            "field");
                continue;
            }

            for (UZ i = 0; i < rows.Count; ++i)
            {
                web_json_value row_json = rows.Items[i];
                if (row_json.Type != JSON_OBJECT)
                {
                    printf("ERROR: Row at index %zu is not an object\n", i);
                    continue;
                }

                web_json_object row = row_json.Object;

                printf("|");

                for (UZ i = 0; i < column_names.Count; ++i)
                {
                    web_json_value column_json = column_names.Items[i];
                    if (column_json.Type != JSON_STRING)
                    {
                        printf("ERROR: Column name at index %zu is not a "
                               "string\n",
                               i);
                        continue;
                    }

                    web_string_view column_name = column_json.String;
                    xmdb::SQL::ColumnType column_type = column_types[i];

                    web_json_value column_value;
                    if (!WebJsonObjectGet(&row, column_name, &column_value))
                    {
                        printf("ERROR: Row at index %zu is missing key "
                               "'" WEB_SV_FMT "'\n",
                               i, WEB_SV_ARG(column_name));
                        continue;
                    }

                    switch (column_value.Type)
                    {
                    case JSON_NUMBER:
                    {
                        F64 number = column_value.Number;
                        F64 integral;
                        if (modf(number, &integral) == 0.0)
                        {
                            printf("%ld", (S64) number);
                        }
                        else
                        {
                            printf("%f", number);
                        }
                        break;
                    }
                    case JSON_STRING:
                    {
                        web_string_view string = column_value.String;
                        printf("\"" WEB_SV_FMT "\"", WEB_SV_ARG(string));
                        break;
                    }
                    case JSON_OBJECT:
                    {
                        web_json_object value_obj = column_value.Object;

                        switch (column_type)
                        {
                        case xmdb::SQL::ColumnType::PNG:
                        {
                            xmdb::Result<xmdb::ImageChunk, ok::String>
                                    parse_result =
                                            parse_image_chunk_from_json_object(
                                                    &arena_adapter, value_obj);

                            if (!parse_result.ok())
                            {
                                const ok::String &error_message =
                                        parse_result.error();
                                printf("ERROR: failed to parse the received "
                                       "JSON object as a valid image chunk: "
                                       "%s\n",
                                       error_message.cstr());
                                continue;
                            }

                            const xmdb::ImageChunk &chunk =
                                    parse_result.unwrap();

                            xmdb::log::debug("Parsed an image chunk:\n\tx => "
                                             "%u\n\ty => %u\n\tw => %u\n\th => "
                                             "%u\n\tdata size => %zu\n",
                                             chunk.x, chunk.y, chunk.width,
                                             chunk.height, chunk.data.count);

                            printf("<image chunk of size %zu>",
                                   chunk.data.count);

                            break;
                        }
                        default:
                        {
                            const char *column_type_str =
                                    xmdb::SQL::column_type_to_string(
                                            column_type);
                            printf("ERROR: got a JSON object for a column of "
                                   "type '%s'\n",
                                   column_type_str);
                            break;
                        }
                        }

                        break;
                    }
                    case JSON_TRUE:
                    {
                        printf("TRUE");
                        break;
                    }
                    case JSON_FALSE:
                    {
                        printf("FALSE");
                        break;
                    }
                    case JSON_NULL:
                    {
                        printf("NULL");
                        break;
                    }
                    case JSON_ARRAY:
                        OK_PANIC("Unexpectedly got a value of JSON array type");
                    }

                    printf("|");
                }

                printf("\n");
            }
        }
    }

    return 0;
}
