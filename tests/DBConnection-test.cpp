#include <gtest/gtest.h>
#include <SQL/SQL.hpp>

#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>

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
    CompiledQuery query{};
    ASSERT_TRUE(compile_and_type_check_source(&arena, source, &query, &error));

    DBPool db_pool{&arena};
    DBConnection db_conn{&db_pool};

    String s = stringify_ir(&arena, &query);
    ok::println(s);

    db_conn.execute(&query);
}
