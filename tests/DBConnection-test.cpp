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
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 2);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->columns_types[1], SQL::COLUMN_TEXT);
}

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

    ASSERT_TRUE(ok);

    ASSERT_FALSE(query_results.error.has_value());
    ASSERT_TRUE(query_results.value.has_value());

    DBTable *results_table = query_results.value.value;
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 2);
    ASSERT_EQ(results_table->rows_count, 1);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->columns_types[1], SQL::COLUMN_TEXT);

    DBValue column1_value = results_table->columns_values[0];

    ASSERT_EQ(column1_value.type, SQL::TYPE_INT);
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>{1});
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>::NONE);

    DBValue column2_value = results_table->columns_values[1];

    ASSERT_EQ(column2_value.type, SQL::TYPE_STRING);
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>{"1"_sv});
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>::NONE);
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
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 2);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->columns_types[1], SQL::COLUMN_TEXT);

    DBValue column1_value = results_table->columns_values[0];

    ASSERT_EQ(column1_value.type, SQL::TYPE_INT);
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>{2});
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>::NONE);

    DBValue column2_value = results_table->columns_values[1];

    ASSERT_EQ(column2_value.type, SQL::TYPE_STRING);
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>{"2"_sv});
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>::NONE);
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
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 2);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->columns_types[1], SQL::COLUMN_TEXT);

    DBValue column1_value = results_table->columns_values[0];

    ASSERT_EQ(column1_value.type, SQL::TYPE_INT);
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>::NONE);

    DBValue column2_value = results_table->columns_values[1];

    ASSERT_EQ(column2_value.type, SQL::TYPE_STRING);
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>::NONE);
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
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 2);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_names[1], "column2"_sv);

    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->columns_types[1], SQL::COLUMN_TEXT);

    DBValue column1_value = results_table->columns_values[0];

    ASSERT_EQ(column1_value.type, SQL::TYPE_INT);
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>{1});
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>::NONE);

    DBValue column2_value = results_table->columns_values[1];

    ASSERT_EQ(column2_value.type, SQL::TYPE_STRING);
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>{"1"_sv});
    ASSERT_EQ(column2_value.u.string.next(), ok::Optional<StringView>::NONE);
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
    ASSERT_EQ(results_table->name, ""_sv);
    ASSERT_EQ(results_table->columns_count, 1);

    ASSERT_EQ(results_table->columns_names[0], "column1"_sv);
    ASSERT_EQ(results_table->columns_types[0], SQL::COLUMN_INTEGER);

    DBValue column1_value = results_table->columns_values[0];

    ASSERT_EQ(column1_value.type, SQL::TYPE_INT);
    ASSERT_EQ(column1_value.u.integer.next(), ok::Optional<S64>::NONE);
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
