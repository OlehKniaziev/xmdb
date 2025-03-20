#include "SQLLexer.hpp"

namespace xmdb {
SQLTokenTable sql_token_table{};

using namespace ok::literals;

ok::Optional<SQLToken> SQLLexer::next() {
    skip_whitespace();

    if (pos >= source.count())
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
    default: {
        if (ok::is_digit(cur))
            OK_TODO();

        StringView ident = take_while(ok::is_alpha);

        Optional<SQLToken::Type> kw_token_type = sql_token_table.get(ident);

        token.type = kw_token_type.or_else(SQLToken::IDENT);
        token.data = ident;

        return token;
    }
    }
}
}; // namespace xmdb
