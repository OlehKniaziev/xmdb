#include <gtest/gtest.h>
#include <SQL/SQL.hpp>

#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>

using namespace ok::literals;
using namespace xmdb;
using namespace xmdb::SQL;

TEST(DBConnection, execute) {
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
    ASSERT_EQ(results_table->column_count, 2);

    ASSERT_EQ(results_table->column_names[0], "column1"_sv);
    ASSERT_EQ(results_table->column_names[1], "column2"_sv);

    ASSERT_EQ(results_table->column_types[0], SQL::COLUMN_INTEGER);
    ASSERT_EQ(results_table->column_types[1], SQL::COLUMN_TEXT);
}
