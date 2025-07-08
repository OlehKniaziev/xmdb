#ifndef XMDB_SQLLEXER_H_
#define XMDB_SQLLEXER_H_

#include <Core/ok.hpp>
#include <Core/SourceLocation.hpp>
#include <cstddef>

using ok::Optional;
using ok::String;
using ok::StringView;
using ok::Table;

#define XMDB_ENUM_SQL_KEYWORDS                                                                                         \
    X("SELECT", KW_SELECT)                                                                                             \
    X("FROM", KW_FROM)                                                                                                 \
    X("WHERE", KW_WHERE)                                                                                               \
    X("USE", KW_USE)                                                                                                   \
    X("INSERT", KW_INSERT)                                                                                             \
    X("INTO", KW_INTO)                                                                                                 \
    X("VALUES", KW_VALUES)                                                                                             \
    X("UPDATE", KW_UPDATE)                                                                                             \
    X("SET", KW_SET)                                                                                                   \
    X("DELETE", KW_DELETE)                                                                                             \
    X("DROP", KW_DROP)                                                                                                 \
    X("TABLE", KW_TABLE)                                                                                               \
    X("DATABASE", KW_DATABASE)                                                                                         \
    X("CREATE", KW_CREATE)                                                                                             \
    X("TRUE", KW_TRUE)                                                                                                 \
    X("FALSE", KW_FALSE)                                                                                               \
    X("NULL", KW_NULL)

#define XMDB_ENUM_SQL_TOKENS                                                                                           \
    X("comma", COMMA)                                                                                                  \
    X("dot", DOT)                                                                                                      \
    X("identifier", IDENT)                                                                                             \
    X("semicolon", SEMICOLON)                                                                                          \
    X("l_paren", L_PAREN)                                                                                              \
    X("r_paren", R_PAREN)                                                                                              \
    X("equals", EQ)                                                                                                    \
    X("greater_than", GT)                                                                                              \
    X("less_than", LT)                                                                                                 \
    X("integer_literal", INTEGER)                                                                                      \
    X("string_literal", STRING)                                                                                        \
    X("unterminated_string", UNTERMINATED_STRING)                                                                      \
    XMDB_ENUM_SQL_KEYWORDS

namespace xmdb::SQL {
extern StringView token_types_pretty[];

struct Token {
    enum Type : uint8_t {
#define X(_t, t) t,
        XMDB_ENUM_SQL_TOKENS
#undef X
    };

    Type type;
    StringView data;
};

static inline SourceLocation locate_token(StringView source, Token token) {
    OK_ASSERT((uintptr_t) token.data.data >= (uintptr_t) source.data);

    ptrdiff_t token_offset = token.data.data - source.data;

    uint32_t line = 1;
    uint32_t column = 1;
    for (ptrdiff_t i = 0; i < token_offset; ++i) {
        // TODO: support DOS-style newlines
        if (source[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    return SourceLocation{
        .line = line,
        .column = column,
        .length = (uint32_t) token.data.count,
    };
}

inline StringView token_type_to_string_view(Token::Type type) {
    return token_types_pretty[type];
}

using TokenTable = Table<StringView, Token::Type>;

extern TokenTable token_table;

struct Lexer {
    explicit Lexer(String source) : source{source.view()}, pos{0} {
    }
    explicit Lexer(StringView source) : source{source}, pos{0} {
    }

    Optional<Token> next();

    template<typename F>
    StringView take_while(F pred) {
        size_t start = pos;

        while (pos < source.count && pred(source[pos])) pos++;

        return source.view(start, pos);
    }

    inline void skip_whitespace() {
        while (pos < source.count && ok::is_whitespace(source.data[pos])) pos++;
    }

    StringView source;
    size_t pos;
};
}; // namespace xmdb::SQL

#endif // XMDB_SQLLEXER_H_
