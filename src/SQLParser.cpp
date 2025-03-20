#include "SQLParser.hpp"
#include <cstddef>

#define TRY(x)                                                                                                         \
    do {                                                                                                               \
        auto _x = (x);                                                                                                 \
        if (!_x.has_value()) return {};                                                                                \
    } while (0)

#define SET_TOKEN_MISMATCH(got, ...)                                                                                   \
    do {                                                                                                               \
        SQLToken::Type expected_arr[] = {__VA_ARGS__};                                                                 \
        ::ok::Slice<SQLToken::Type> expected = {expected_arr, OK_ARR_LEN(expected_arr)};                               \
        set_token_mismatch((got), expected);                                                                           \
    } while (0)

using namespace ok::literals;

namespace xmdb {
namespace {
SourceLocation locate_token(StringView source, SQLToken token) {
    OK_ASSERT((uintptr_t)token.data.data >= (uintptr_t)source.data);

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
}; // namespace

SQLParser::SQLParser(ok::ArenaAllocator* arena, StringView source) : arena{arena}, source{source} {
    auto tokens = ok::List<SQLToken>::alloc(arena);
    SQLLexer lexer{source};

    for (auto token = lexer.next(); token.has_value(); token = lexer.next()) {
        tokens.push(token.value);
    }

    this->tokens = tokens.slice();
}

Optional<SQLSelectStmt*> SQLParser::select_stmt() {
    TRY(expect(SQLToken::KW_SELECT));

    auto exprs = ok::List<SQLExpr*>::alloc(arena);

    while (true) {
        auto expr = expression();
        if (!expr.has_value()) return {};

        exprs.push(expr.value);

        if (!try_expect(SQLToken::COMMA)) break;
    }

    TRY(expect(SQLToken::KW_FROM));

    auto table_expr = expression();
    if (!table_expr.has_value()) return {};

    auto* select_stmt = arena->alloc<SQLSelectStmt>();
    select_stmt->exprs = exprs.slice();
    select_stmt->table = table_expr.value;

    TRY(expect(SQLToken::SEMICOLON));

    return select_stmt;
}

Optional<SQLExpr*> SQLParser::expression() {
    auto token = get_cur_token_or_signal_eof();
    if (!token.has_value()) return {};

    switch (token.value.type) {
    case SQLToken::IDENT: {
        pos++;
        return SQLExprIdentifier::alloc(arena, token.value.data);
    }
    default: OK_TODO();
    }
}

bool SQLParser::cur_token_is(SQLToken::Type token_type) const {
    return pos < tokens.count && tokens[pos].type == token_type;
}

bool SQLParser::try_expect(SQLToken::Type token_type) {
    if (cur_token_is(token_type)) {
        pos++;
        return true;
    }

    return false;
}

Optional<SQLToken> SQLParser::expect(SQLToken::Type token_type) {
    if (is_eof()) {
        set_eof();
        return {};
    }

    if (cur_token_is(token_type)) {
        return tokens[pos++];
    }

    SET_TOKEN_MISMATCH(tokens[pos], token_type);

    return {};
}

Optional<SQLToken> SQLParser::get_cur_token_or_signal_eof() {
    if (is_eof()) {
        set_eof();
        return {};
    }

    return tokens[pos];
}

void SQLParser::set_token_mismatch(SQLToken got, ok::Slice<SQLToken::Type> expected) {
    OK_ASSERT(expected.count != 0);

    auto token_location = locate_token(source, got);

    String message;

    auto got_sv = sql_token_type_to_string_view(got.type);
    if (expected.count == 1) {
        auto expected_sv = sql_token_type_to_string_view(expected[0]);
        message = String::format(arena, "expected a token of type " OK_SV_FMT ", but got " OK_SV_FMT " instead",
                                 OK_SV_ARG(expected_sv), OK_SV_ARG(got_sv));
    } else {
        message = String::alloc(arena, "expected a token of types ");

        for (size_t i = 0; i < expected.count; i++) {
            auto expected_sv = sql_token_type_to_string_view(expected[i]);
            message.format_append(OK_SV_FMT, OK_SV_ARG(expected_sv));
            if (i != expected.count - 1) message.append("or "_sv);
        }

        message.format_append(", but got " OK_SV_FMT " instead", OK_SV_ARG(got_sv));
    }

    error = Error{message, token_location};
}

void SQLParser::set_eof() {
    SourceLocation location;
    if (tokens.count > 0) location = locate_token(source, tokens[tokens.count - 1]);
    else
        location = {
                .line = 1,
                .column = 1,
                .length = 0,
        };

    auto message = String::alloc(arena, "unexpected EOF");
    error = Error{message, location};
}
}; // namespace xmdb
