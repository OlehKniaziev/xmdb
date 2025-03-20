#ifndef XMDB_SQLLEXER_H_
#define XMDB_SQLLEXER_H_

#include "ok.hpp"

using ok::String;
using ok::StringView;
using ok::Table;
using ok::Optional;

#define XMDB_ENUM_SQL_KEYWORDS                                                                                           \
    X("SELECT", KW_SELECT)                                                                                             \
    X("FROM", KW_FROM)

#define XMDB_ENUM_SQL_TOKENS \
    X("comma", COMMA) \
    X("dot", DOT) \
    X("identifier", IDENT) \
    X(";", SEMICOLON) \
    XMDB_ENUM_SQL_KEYWORDS

namespace xmdb {
struct SQLToken {
    enum Type : uint8_t {
#define X(_t, t) t,
        XMDB_ENUM_SQL_TOKENS
#undef X
    };

    void format_to(String*);

    Type type;
    StringView data;
};

using SQLTokenTable = Table<StringView, SQLToken::Type>;

extern SQLTokenTable sql_token_table;

struct SQLLexer {
    explicit SQLLexer(String source) : source{source.copy(ok::static_allocator)}, pos{0} {}
    explicit SQLLexer(StringView source) : source{source.to_string(ok::static_allocator)}, pos{0} {}

    Optional<SQLToken> next();

    template <typename F>
    StringView take_while(F pred) {
        size_t start = pos;

        while (pos < source.count() && pred(source[pos])) pos++;

        return source.view(start, pos);
    }

    inline void skip_whitespace() {
        while (pos < source.count() && ok::is_whitespace(source.data[pos])) pos++;
    }

    String source;
    size_t pos;
};
};

#endif // XMDB_SQLLEXER_H_
