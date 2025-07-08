#include <gtest/gtest.h>
#include <SQL/SQL.hpp>

#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>

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

    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db};

    bool ok = compile_and_execute_source(&arena,
                                         &db_conn,
                                         source,
                                         &query_results,
                                         &error);

    ASSERT_TRUE(ok);

    ASSERT_TRUE(query_results.ok);
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

    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db};

    bool ok = compile_and_execute_source(&arena,
                                         &db_conn,
                                         source,
                                         &query_results,
                                         &error);

    ASSERT_TRUE(ok);

    ASSERT_TRUE(query_results.ok);
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

    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db};

    bool ok = compile_and_execute_source(&arena,
                                         &db_conn,
                                         source,
                                         &query_results,
                                         &error);

    ASSERT_TRUE(ok);

    ASSERT_TRUE(query_results.ok);
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

    DBPool db_pool{&arena};
    DBDescriptor *default_db = db_pool.get_db("default"_sv);
    DBConnection db_conn{&db_pool, default_db};

    bool ok = compile_and_execute_source(&arena,
                                         &db_conn,
                                         source,
                                         &query_results,
                                         &error);

    ASSERT_TRUE(ok);

    ASSERT_TRUE(query_results.ok);
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
