#include "SQLParser.hpp"

#define TRY(x)                                                                                                         \
    do {                                                                                                               \
        auto _x = (x);                                                                                                 \
        if (!_x.has_value()) return {};                                                                                \
    } while (0)

namespace xmdb {
SQLParser::SQLParser(ok::ArenaAllocator* arena, StringView source) : arena{arena} {
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
    if (cur_token_is(token_type)) {
        return tokens[pos++];
    }

    OK_TODO();
    return {};
}

Optional<SQLToken> SQLParser::get_cur_token_or_signal_eof() const {
    if (pos >= tokens.count) OK_TODO();

    return tokens[pos];
}
}; // namespace xmdb
