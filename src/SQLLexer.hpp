#ifndef XMDB_SQLLEXER_H_
#define XMDB_SQLLEXER_H_

#include "ok.hpp"

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
    X("DATABASE", KW_DATABASE)

#define XMDB_ENUM_SQL_TOKENS                                                                                           \
    X("comma", COMMA)                                                                                                  \
    X("dot", DOT)                                                                                                      \
    X("identifier", IDENT)                                                                                             \
    X("semicolon", SEMICOLON)                                                                                          \
    X("l_paren", L_PAREN)                                                                                              \
    X("r_paren", R_PAREN)                                                                                              \
    X("equals", EQ)                                                                                                    \
    XMDB_ENUM_SQL_KEYWORDS

namespace xmdb {
extern StringView sql_token_types_pretty[];

struct SQLToken {
    enum Type : uint8_t {
#define X(_t, t) t,
        XMDB_ENUM_SQL_TOKENS
#undef X
    };

    Type type;
    StringView data;
};

inline StringView sql_token_type_to_string_view(SQLToken::Type type) {
    return sql_token_types_pretty[type];
}

using SQLTokenTable = Table<StringView, SQLToken::Type>;

extern SQLTokenTable sql_token_table;

struct SQLLexer {
    explicit SQLLexer(String source) : source{source.view()}, pos{0} {
    }
    explicit SQLLexer(StringView source) : source{source}, pos{0} {
    }

    Optional<SQLToken> next();

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
}; // namespace xmdb

#endif // XMDB_SQLLEXER_H_
