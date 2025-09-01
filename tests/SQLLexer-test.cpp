#include <Core/ok.hpp>
#include <gtest/gtest.h>
#include "SQL/Lexer.hpp"

using namespace xmdb::SQL;
using namespace ok::literals;

TEST(Lexer, Next) {
    Lexer lexer{"SELECT id, name, 123 FROM Users"_sv};

    EXPECT_EQ(lexer.next().get().type, Token::KW_SELECT);

    auto id_token = lexer.next().get();
    EXPECT_EQ(id_token.type, Token::IDENT);
    EXPECT_EQ(id_token.data, "id"_sv);

    EXPECT_EQ(lexer.next().get().type, Token::COMMA);

    auto name_token = lexer.next().get();
    EXPECT_EQ(name_token.type, Token::IDENT);
    EXPECT_EQ(name_token.data, "name"_sv);

    EXPECT_EQ(lexer.next().get().type, Token::COMMA);

    auto int_token = lexer.next().get();
    EXPECT_EQ(int_token.type, Token::INTEGER);
    EXPECT_EQ(int_token.data, "123"_sv);

    EXPECT_EQ(lexer.next().get().type, Token::KW_FROM);

    auto users_token = lexer.next().get();
    EXPECT_EQ(users_token.type, Token::IDENT);
    EXPECT_EQ(users_token.data, "Users"_sv);
}

TEST(Lexer, Comments) {
    Lexer lexer{"-- SELECT +/*-/123error fro t;ebl\nSELECT"_sv};

    EXPECT_EQ(lexer.next().get().type, Token::KW_SELECT);
}
