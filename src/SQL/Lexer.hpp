#ifndef XMDB_SQLLEXER_H_
#define XMDB_SQLLEXER_H_

#include <Core/ok.hpp>
#include <Core/SourceLocation.hpp>
#include <stddef.h>

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
    X("USER", KW_USER) \
    X("TRUE", KW_TRUE)                                                                                                 \
    X("FALSE", KW_FALSE)                                                                                               \
    X("NULL", KW_NULL) \
    X("ALTER", KW_ALTER)

#define XMDB_ENUM_SQL_TOKENS                                                                                           \
    X("comma", COMMA)                                                                                                  \
    X("dot", DOT)                                                                                                      \
    X("identifier", IDENT)                                                                                             \
    X("semicolon", SEMICOLON)                                                                                          \
    X("star", STAR) \
    X("l_paren", L_PAREN)                                                                                              \
    X("r_paren", R_PAREN)                                                                                              \
    X("equals", EQ)                                                                                                    \
    X("greater_than", GT)                                                                                              \
    X("less_than", LT)                                                                                                 \
    X("integer_literal", INTEGER)                                                                                      \
    X("string_literal", STRING)                                                                                        \
    X("unterminated_string", UNTERMINATED_STRING)                                                                      \
    X("illegal", ILLEGAL)                                                                                              \
    XMDB_ENUM_SQL_KEYWORDS

namespace xmdb::SQL {
extern StringView token_types_pretty[];

/**
 * @brief Represents a single token in SQL source code.
 */
struct Token {
    /**
     * @brief Types of SQL tokens.
     */
    enum Type : uint8_t {
#define X(_t, t) t,
        XMDB_ENUM_SQL_TOKENS
#undef X
    };

    Type type;       ///< The type of the token.
    StringView data; ///< The literal text of the token in the source.
};

/**
 * @brief Calculates the source location of a given token.
 * @param source The full source string.
 * @param token The token to locate.
 * @return The source location information.
 */
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

/**
 * @brief Converts a token type to its human-readable string representation.
 * @param type The token type.
 * @return A string view of the type name.
 */
inline StringView token_type_to_string_view(Token::Type type) {
    return token_types_pretty[type];
}

using TokenTable = Table<StringView, Token::Type>;

extern TokenTable token_table;

/**
 * @brief Lexical analyzer for SQL source code.
 */
struct Lexer {
    /**
     * @brief Constructs a Lexer from an ok::String.
     * @param source The source string.
     */
    explicit Lexer(String source) : source{source.view()}, pos{0} {
    }

    /**
     * @brief Constructs a Lexer from an ok::StringView.
     * @param source The source string view.
     */
    explicit Lexer(StringView source) : source{source}, pos{0} {
    }

    /**
     * @brief Retrieves the next token from the source.
     * @return An optional containing the next token, or empty if end of source.
     */
    Optional<Token> next();

    /**
     * @brief Consumes characters while a predicate is true and returns them as a view.
     * @tparam F Type of the predicate function.
     * @param pred The predicate function.
     * @return A string view of the consumed characters.
     */
    template<typename F>
    StringView take_while(F pred) {
        size_t start = pos;

        skip_while(pred);

        return source.view(start, pos);
    }

    /**
     * @brief Skips characters while a predicate is true.
     * @tparam F Type of the predicate function.
     * @param pred The predicate function.
     */
    template <typename F>
    void skip_while(F pred) {
        while (pos < source.count && pred(source[pos])) ++pos;
    }

    /**
     * @brief Skips all whitespace characters.
     */
    inline void skip_whitespace() {
        skip_while(ok::is_whitespace);
    }

    StringView source; ///< The source string being analyzed.
    size_t pos;        ///< Current position in the source string.
};
}; // namespace xmdb::SQL

#endif // XMDB_SQLLEXER_H_
