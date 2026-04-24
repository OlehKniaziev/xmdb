#include "Lexer.hpp"

#ifdef OK_NO_STDLIB
char toupper(char c)
{
    return c & ~(1 << 5);
}
#else
#include <cctype>
#endif // OK_NO_STDLIB

namespace xmdb::SQL
{
TokenTable token_table{};

StringView token_types_pretty[] = {
#define X(str, tok) StringView{str},
        XMDB_ENUM_SQL_TOKENS
#undef X
};

using namespace ok::literals;

namespace
{
inline bool is_valid_ident_char(char c)
{
    return ok::is_alpha(c) || ok::is_digit(c) || c == '_';
}
}; // namespace

ok::Optional<Token> Lexer::next()
{
    skip_whitespace();

    if (pos >= source.count) return {};

    char cur;

    Token token{};

    switch ((cur = source.data[pos]))
    {
    case '-':
    {
        ++pos;
        if (source.count > pos && source.data[pos] == '-')
        {
            skip_while([](U8 c) { return c != '\n'; });
            return next();
        }

        token.type = Token::ILLEGAL;
        token.data = source.view(pos - 1, pos);
        return token;
    }
    case ',':
    {
        token.type = Token::COMMA;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '.':
    {
        token.type = Token::DOT;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '*':
    {
        token.type = Token::STAR;
        token.data = source.view(pos, pos + 1);
        ++pos;
        return token;
    }
    case ';':
    {
        token.type = Token::SEMICOLON;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '(':
    {
        token.type = Token::L_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case ')':
    {
        token.type = Token::R_PAREN;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '=':
    {
        token.type = Token::EQ;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '<':
    {
        token.type = Token::LT;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '>':
    {
        token.type = Token::GT;
        token.data = source.view(pos, pos + 1);
        pos++;
        return token;
    }
    case '\'':
    case '"':
    {
        auto quote = cur;

        ++pos;

        auto start = pos;
        while (pos < source.count && source.data[pos] != quote) ++pos;

        if (pos >= source.count)
        {
            token.type = Token::UNTERMINATED_STRING;
            token.data = source.view(start, pos);
            return token;
        }

        token.type = Token::STRING;
        token.data = source.view(start, pos);

        ++pos;

        return token;
    }
    default:
    {
        if (ok::is_digit(cur))
        {
            StringView integer = take_while(ok::is_digit);
            token.type = Token::INTEGER;
            token.data = integer;
            return token;
        }

        if (!is_valid_ident_char(cur))
        {
            ++pos;
            token.type = Token::ILLEGAL;
            token.data = source.view(pos, pos + 1);
            return token;
        }

        StringView ident_sv = take_while(is_valid_ident_char);

        String ident = ident_sv.to_string(ok::temp_allocator());
        for (UZ i = 0; i < ident.count(); ++i)
        {
            ident[i] = toupper(ident[i]);
        }

        Optional<Token::Type> kw_token_type = token_table.get(ident);

        token.type = kw_token_type.or_else(Token::IDENT);
        token.data = ident_sv;

        return token;
    }
    }
}
}; // namespace xmdb::SQL
