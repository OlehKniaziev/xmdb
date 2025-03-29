#include <gtest/gtest.h>
#include <SQL/sema.hpp>
#include <SQL/Parser.hpp>

using namespace ok::literals;
using namespace xmdb::SQL;

TEST(sema, basic) {
    ok::ArenaAllocator arena{};
    auto source = R"sql(CREATE TABLE MyTable (
        column1 int,
        column2 text
    );
    SELECT column1, column1, column2 FROM MyTable;)sql"_sv;
    Parser parser{&arena, source};

    auto query = parser.query();
    ASSERT_TRUE(query.has_value());

    SemaContext sema_ctx{&arena};

    ASSERT_TRUE(sema_check_query(&query.value, &sema_ctx));
}
