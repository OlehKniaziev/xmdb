#include "Lexer.hpp"

namespace xmdb::SQL {
TokenTable token_table{};

StringView token_types_pretty[] = {
#define X(str, tok) [Token::tok] = StringView{str},
        XMDB_ENUM_SQL_TOKENS
#undef X
};

using namespace ok::literals;

namespace {
inline bool is_valid_ident_char(char c) {
    return ok::is_alpha(c) || ok::is_digit(c) || c == '_';
}
}; // namespace

ok::Optional<Token> Lexer::next() {
    skip_whitespace();

    if (pos >= source.count) return {};

    char cur;

    Token token{};

    switch ((cur = source.data[pos])) {
    case ',': {
        token.type = Token::COMMA;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '.': {
        token.type = Token::DOT;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case ';': {
        token.type = Token::SEMICOLON;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '(': {
        token.type = Token::L_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case ')': {
        token.type = Token::R_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '=': {
        token.type = Token::EQ;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '\'':
    case '"': {
        auto quote = cur;

        ++pos;

        auto start = pos;
        while (pos < source.count && source.data[pos] != quote) ++pos;

        if (pos >= source.count) {
            token.type = Token::UNTERMINATED_STRING;
            token.data = source.view(start, pos);
            return token;
        }

        token.type = Token::STRING;
        token.data = source.view(start, pos);

        ++pos;

        return token;
    }
    default: {
        if (ok::is_digit(cur)) {
            StringView integer = take_while(ok::is_digit);
            token.type = Token::INTEGER;
            token.data = integer;
            return token;
        }

        OK_ASSERT(is_valid_ident_char(cur));

        StringView ident = take_while(is_valid_ident_char);

        Optional<Token::Type> kw_token_type = token_table.get(ident);

        token.type = kw_token_type.or_else(Token::IDENT);
        token.data = ident;

        return token;
    }
    }
}
}; // namespace xmdb
