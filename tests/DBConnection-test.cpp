#include <SQL/SQL.hpp>
#include <gtest/gtest.h>

#include <Core/Core.hpp>
#include <Core/DBConnection.hpp>
#include <Core/DBPool.hpp>

using namespace ok::literals;
using namespace xmdb;
using namespace xmdb::SQL;

TEST(DBConnection, execute_create_and_select_on_empty_table) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    SELECT column1, column2 FROM MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 2);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names()[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);
    ASSERT_EQ(results_table->columns_types()[1], SQL::ColumnType::TEXT);
}

TEST(DBConnection, select_empty_png_column) {
#define PIXEL_DATA "abcdabcdabcdabcdabababab"
    ok::ArenaAllocator arena{};
    StringView source = "CREATE TABLE MyTable ("
        "id int,"
        "img PNG"
    ");"
    "INSERT INTO MyTable(id, img) VALUES (1, RGB(2, 2, \"" PIXEL_DATA "\"));"
    "SELECT img FROM MyTable;"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->columns_count(), 1);

    DBTableOutlet outlet{results_table};
    DBTableStream stream = outlet.column_stream(&arena, 0);

    ok::Optional<Value> val_opt = stream.next();
    ASSERT_TRUE(val_opt.has_value());

    Value val = val_opt.get();

    ASSERT_EQ(val.type(), Value::Type::IMAGE_CHUNK);

    ImageChunk *image_chunk = val.as_chunk();

    ASSERT_EQ(image_chunk->x, 0);
    ASSERT_EQ(image_chunk->y, 0);
    ASSERT_EQ(image_chunk->width, 2);
    ASSERT_EQ(image_chunk->height, 2);
    ASSERT_EQ(image_chunk->pixel_format, PixelFormat::RGB);

    ok::Slice<U8> expected_pixel_data = from_hex_string(&arena, ok::StringView{PIXEL_DATA}).get();

    ASSERT_EQ(expected_pixel_data.count, image_chunk->data.count);

    for (UZ pi = 0; pi < image_chunk->data.count; ++pi) {
        U8 px = image_chunk->data[pi];
        ASSERT_EQ(px, expected_pixel_data[pi]);
    }
}

struct MallocAllocator : public ok::ArenaAllocator {
    void *raw_alloc(UZ size) override {
        return calloc(1, size);
    }

    void raw_dealloc(void *ptr, UZ size) override {
        (void) size;
        ::free(ptr);
    }
};

TEST(DBConnection, execute_create_insert_and_select_on_table_with_one_row) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    INSERT INTO MyTable(column1, column2) VALUES(1, "1");
    SELECT column1, column2 FROM MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 2);
    ASSERT_EQ(results_table->rows_count(), 1);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names()[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);
    ASSERT_EQ(results_table->columns_types()[1], SQL::ColumnType::TEXT);

    DBTableOutlet results_outlet{results_table};

    DBTableStream column1_value = results_outlet.column_stream(&arena, 0);

    ok::Optional<Value> next_value = column1_value.next();

    ASSERT_TRUE(next_value);
    ASSERT_EQ(next_value.get().type(), Value::Type::INT);
    ASSERT_EQ(next_value.get().as_int(), 1);

    ASSERT_FALSE(column1_value.next());

    DBTableStream column2_value = results_outlet.column_stream(&arena, 1);

    next_value = column2_value.next();

    ASSERT_TRUE(next_value);
    ASSERT_EQ(next_value.get().type(), Value::Type::STRING);

    FixedString val_fs = next_value.get().as_string();
    ok::StringView val = val_fs.view();
    ASSERT_EQ(val, "1"_sv);

    ASSERT_FALSE(column2_value.next());
}

TEST(DBConnection, execute_create_insert_update_and_select_on_table_with_one_row) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    INSERT INTO MyTable(column1, column2) VALUES(1, "1");
    UPDATE MyTable SET column1 = 2, column2 = "2";
    SELECT column1, column2 FROM MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 2);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names()[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);
    ASSERT_EQ(results_table->columns_types()[1], SQL::ColumnType::TEXT);

    DBTableOutlet results_outlet{results_table};

    DBTableStream column1_stream = results_outlet.column_stream(&arena, 0);
    DBTableStream column2_stream = results_outlet.column_stream(&arena, 1);

    ok::Optional<Value> val1 = column1_stream.next();

    ASSERT_TRUE(val1);
    ASSERT_EQ(val1.get().type(), Value::Type::INT);
    ASSERT_EQ(val1.get().as_int(), 2);

    ASSERT_FALSE(column1_stream.next());

    ok::Optional<Value> val2 = column2_stream.next();

    ASSERT_TRUE(val2);

    FixedString val2_fs = val2.get().as_string();

    ASSERT_EQ(val2.get().type(), Value::Type::STRING);
    ASSERT_EQ(val2_fs, "2"_sv);

    ASSERT_FALSE(column2_stream.next());
}

TEST(DBConnection, execute_create_insert_delete_and_select_on_table_with_one_row) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    INSERT INTO MyTable(column1, column2) VALUES(1, "1");
    DELETE FROM MyTable;
    SELECT column1, column2 FROM MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 2);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names()[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);
    ASSERT_EQ(results_table->columns_types()[1], SQL::ColumnType::TEXT);

    DBTableOutlet outlet{results_table};

    DBTableStream column1_stream = outlet.column_stream(&arena, 0);
    DBTableStream column2_stream = outlet.column_stream(&arena, 1);

    ASSERT_FALSE(column1_stream.next());
    ASSERT_FALSE(column2_stream.next());
}

TEST(DBConnection, create_new_db_and_execute_create_insert_and_select_on_table_with_one_row) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(
    CREATE DATABASE MyDb;
    USE MyDb;

    CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    INSERT INTO MyTable(column1, column2) VALUES(1, "1");
    SELECT column1, column2 FROM MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_EQ(db_conn.db->name, "MyDb"_sv);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 2);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names()[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);
    ASSERT_EQ(results_table->columns_types()[1], SQL::ColumnType::TEXT);

    DBTableOutlet outlet{results_table};

    DBTableStream column1_stream = outlet.column_stream(&arena, 0);
    DBTableStream column2_stream = outlet.column_stream(&arena, 1);

    ok::Optional<Value> val1 = column1_stream.next();
    ok::Optional<Value> val2 = column2_stream.next();

    ASSERT_TRUE(val1);

    ASSERT_EQ(val1.get().type(), Value::Type::INT);
    ASSERT_EQ(val1.get().as_int(), 1);

    ASSERT_FALSE(column1_stream.next());

    ASSERT_TRUE(val2);

    FixedString val2_fs = val2.get().as_string();

    ASSERT_EQ(val2.get().type(), Value::Type::STRING);
    ASSERT_EQ(val2_fs, "1"_sv);

    ASSERT_FALSE(column2_stream.next());
}

TEST(DBConnection, insert_and_select_multiple_rows) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(
    CREATE TABLE TAB(id int);
    INSERT INTO TAB(id) VALUES (1), (2), (3);
    SELECT id FROM TAB;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    DBTable *results_table = query_results.value.get();
    DBTableOutlet outlet{results_table};

    DBTableStream stream = outlet.column_stream(&arena, 0);

    Optional<Value> val_opt = stream.next();
    ASSERT_TRUE(val_opt.has_value());

    Value val = val_opt.get();
    ASSERT_EQ(val.type(), Value::Type::INT);
    ASSERT_EQ(val.as_int(), 1);

    val_opt = stream.next();
    ASSERT_TRUE(val_opt.has_value());

    val = val_opt.get();
    ASSERT_EQ(val.type(), Value::Type::INT);
    ASSERT_EQ(val.as_int(), 2);

    val_opt = stream.next();
    ASSERT_TRUE(val_opt.has_value());

    val = val_opt.get();
    ASSERT_EQ(val.type(), Value::Type::INT);
    ASSERT_EQ(val.as_int(), 3);

    ASSERT_FALSE(stream.next());
}

TEST(DBConnection, create_and_drop_empty_table) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(
    CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    DROP TABLE MyTable;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(!query_results.value.has_value());

    ASSERT_EQ(default_db->tables.count, 0);
}

TEST(DBConnection, create_and_drop_empty_database) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(
    CREATE DATABASE DB;
    DROP DATABASE DB;)sql"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(!query_results.value.has_value());

    for (DBDescriptor *db = db_pool.db_descriptors; db != nullptr; db = db->next) {
        ASSERT_NE(db->name, "DB"_sv);
    }
}

TEST(DBConnection, execute_multiple_queries_with_the_same_connection) {
    ok::ArenaAllocator arena{};
    StringView source = "CREATE DATABASE DB;"_sv;

    String error{};

    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();
    ASSERT_FALSE(query_results.error.has_value());

    source = "USE DB;"_sv;
    ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();
    ASSERT_FALSE(query_results.error.has_value());

    source = "CREATE TABLE MyTable (column1 int);"_sv;
    ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();
    ASSERT_FALSE(query_results.error.has_value());

    source = "SELECT column1 FROM MyTable;"_sv;
    ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();
    ASSERT_FALSE(query_results.error.has_value());

    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name(), ""_sv);
    ASSERT_EQ(results_table->columns_count(), 1);

    ASSERT_EQ(results_table->columns_names()[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_types()[0], SQL::ColumnType::INTEGER);

    DBTableOutlet outlet{results_table};
    DBTableStream stream = outlet.column_stream(&arena, 0);
    ASSERT_FALSE(stream.next());
}

TEST(DBConnection, user_permissions) {
    ok::ArenaAllocator arena{};
    StringView source = "create table tab (x int);"_sv;

    String error{};

    QueryResults query_results{};

    DBUser user = {"user"_sv, "user"_sv, 0};
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &user};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_FALSE(ok) << error.cstr();
    ASSERT_TRUE(query_results.error.has_value());

    ASSERT_EQ(error, "0:0: user user does not have WRITE permissions"_sv);

    user.perm |= PERM_WRITE;

    ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();

    source = "select * from tab;"_sv;

    ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_FALSE(ok);
    ASSERT_EQ(error, "0:0: user user does not have READ permissions"_sv);
}

TEST(DBConnection, create_new_user) {
    ok::ArenaAllocator arena{};
    StringView source = "create user some_user;"_sv;

    String error{};

    QueryResults query_results{};

    DBUser user = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &user};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();

    DBUser *usr = nullptr;

    while (default_db->users != nullptr) {
        if (default_db->users->name == "some_user"_sv) {
            usr = default_db->users;
            break;
        }

        default_db->users = default_db->users->next;
    }

    ASSERT_NE(usr, nullptr);
}

TEST(DBConnection, alter_new_user) {
    ok::ArenaAllocator arena{};
    StringView source = "create user some_user; alter user some_user set PASSWORD = 'pass';"_sv;

    String error{};

    QueryResults query_results{};

    DBUser user = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &user};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_TRUE(ok) << error.cstr();

    Optional<DBUser *> some_user = default_db->find_user("some_user"_sv);
    ASSERT_TRUE(some_user.has_value());

    ASSERT_EQ(some_user.value->sha256_password_digest, sha256_digest("pass"_sv));
}

TEST(DBConnection, create_user_requires_admin_permissions) {
    ok::ArenaAllocator arena{};
    StringView source = "CREATE USER new_user;"_sv;

    String error{};
    QueryResults query_results{};

    DBUser user = {"regular"_sv, ""_sv, PERM_READ | PERM_WRITE};
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &user};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_FALSE(ok);
    ASSERT_TRUE(error.ends_with("does not have ADMIN permissions")) << error.cstr();
}

TEST(DBConnection, create_duplicate_user_fails) {
    ok::ArenaAllocator arena{};
    StringView source = R"sql(
    CREATE USER dup_user;
    CREATE USER dup_user;)sql"_sv;

    String error{};
    QueryResults query_results{};

    DBUser admin = DBUser::admin();
    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db, &admin};

    bool ok = compile_and_execute_source(&arena, &db_conn, source, &query_results, &error);

    ASSERT_FALSE(ok);
    ASSERT_TRUE(error.ends_with("already exists")) << error.cstr();
}
