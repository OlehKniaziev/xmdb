#include <gtest/gtest.h>
#include "SQLLexer.hpp"
#include "ok.hpp"

using namespace xmdb;
using namespace ok::literals;

TEST(Lexer, Next) {
    SQLLexer lexer{"SELECT id, name FROM Users"_sv};

    EXPECT_EQ(lexer.next().get().type, SQLToken::KW_SELECT);

    auto id_token = lexer.next().get();
    EXPECT_EQ(id_token.type, SQLToken::IDENT);
    EXPECT_EQ(id_token.data, "id"_sv);

    EXPECT_EQ(lexer.next().get().type, SQLToken::COMMA);

    auto name_token = lexer.next().get();
    EXPECT_EQ(name_token.type, SQLToken::IDENT);
    EXPECT_EQ(name_token.data, "name"_sv);

    EXPECT_EQ(lexer.next().get().type, SQLToken::KW_FROM);

    auto users_token = lexer.next().get();
    EXPECT_EQ(users_token.type, SQLToken::IDENT);
    EXPECT_EQ(users_token.data, "Users"_sv);
}
