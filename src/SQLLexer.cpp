#include "SQLLexer.hpp"

namespace xmdb {
SQLTokenTable sql_token_table{};

StringView sql_token_types_pretty[] = {
#define X(str, tok) [SQLToken::tok] = StringView{str},
        XMDB_ENUM_SQL_TOKENS
#undef X
};

using namespace ok::literals;

namespace {
inline bool is_valid_ident_char(char c) {
    return ok::is_alpha(c) || ok::is_digit(c) || c == '_';
}
};

ok::Optional<SQLToken> SQLLexer::next() {
    skip_whitespace();

    if (pos >= source.count)
        return {};

    char cur;

    SQLToken token{};

    switch ((cur = source.data[pos])) {
    case ',': {
        token.type = SQLToken::COMMA;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '.': {
        token.type = SQLToken::DOT;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case ';': {
        token.type = SQLToken::SEMICOLON;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '(': {
        token.type = SQLToken::L_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case ')': {
        token.type = SQLToken::R_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    default: {
        if (ok::is_digit(cur))
            OK_TODO();

        StringView ident = take_while(is_valid_ident_char);

        Optional<SQLToken::Type> kw_token_type = sql_token_table.get(ident);

        token.type = kw_token_type.or_else(SQLToken::IDENT);
        token.data = ident;

        return token;
    }
    }
}
}; // namespace xmdb
