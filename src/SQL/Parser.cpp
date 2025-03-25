#include "Parser.hpp"
#include <cstddef>

#define TRY(x)                                                                                                         \
    do {                                                                                                               \
        auto _x = (x);                                                                                                 \
        if (!_x.has_value()) return {};                                                                                \
    } while (0)

#define SET_TOKEN_MISMATCH(got, ...)                                                                                   \
    do {                                                                                                               \
        Token::Type expected_arr[] = {__VA_ARGS__};                                                                    \
        ::ok::Slice<Token::Type> expected = {expected_arr, OK_ARR_LEN(expected_arr)};                                  \
        set_token_mismatch((got), expected);                                                                           \
    } while (0)

using namespace ok::literals;

namespace xmdb::SQL {
namespace {
SourceLocation locate_token(StringView source, Token token) {
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

Optional<ExprSelect*> parse_select_expr(Parser* p) {
    TRY(p->expect(Token::KW_SELECT));

    auto exprs = ok::List<Expr*>::alloc(p->arena);

    while (true) {
        auto expr = p->expression();
        TRY(expr);

        exprs.push(expr.value);

        if (!p->try_expect(Token::COMMA)) break;
    }

    TRY(p->expect(Token::KW_FROM));

    auto table_expr = p->expression();
    TRY(table_expr);

    TRY(p->expect(Token::SEMICOLON));

    return ExprSelect::alloc(p->arena, exprs.slice(), table_expr.value);
}

Optional<Expr*> parse_expression_prim(Parser* parser) {
    auto token = parser->get_cur_token_or_signal_eof();
    if (!token.has_value()) return {};

    switch (token.value.type) {
    case Token::IDENT: {
        ++parser->pos;
        return ExprIdentifier::alloc(parser->arena, token.value.data);
    }
    case Token::INTEGER: {
        ++parser->pos;
        int64_t integer;
        OK_ASSERT(ok::parse_int64(token.value.data, &integer));
        return ExprInteger::alloc(parser->arena, integer);
    }
    case Token::STRING: {
        ++parser->pos;
        return ExprString::alloc(parser->arena, token.value.data.to_string(parser->arena));
    }
    case Token::KW_TRUE: {
        ++parser->pos;
        return Expr::true_literal;
    }
    case Token::KW_FALSE: {
        ++parser->pos;
        return Expr::false_literal;
    }
    case Token::KW_NULL: {
        ++parser->pos;
        return Expr::null_literal;
    }
    case Token::KW_SELECT: return parse_select_expr(parser).upcast<Expr>();

    default: OK_TODO();
    }
}
}; // namespace

Parser::Parser(ok::ArenaAllocator* arena, StringView source) : arena{arena}, source{source} {
    auto tokens = ok::List<Token>::alloc(arena);
    Lexer lexer{source};

    for (auto token = lexer.next(); token.has_value(); token = lexer.next()) {
        tokens.push(token.value);
    }

    this->tokens = tokens.slice();
}

Optional<UseStmt*> Parser::use_stmt() {
    TRY(expect(Token::KW_USE));

    auto database = expect(Token::IDENT);
    TRY(database);

    return UseStmt::alloc(arena, database.value.data);
}

Optional<InsertStmt*> Parser::insert_stmt() {
    TRY(expect(Token::KW_INSERT));
    TRY(expect(Token::KW_INTO));

    auto table_expr = expression();
    TRY(table_expr);

    TRY(expect(Token::L_PAREN));

    auto columns = ok::List<ok::String>::alloc(arena);

    while (true) {
        auto column = expect(Token::IDENT);
        TRY(column);
        columns.push(column.value.data.to_string(arena));

        if (!try_expect(Token::COMMA)) break;
    }

    TRY(expect(Token::R_PAREN));

    TRY(expect(Token::KW_VALUES));

    auto values = ok::List<Expr*>::alloc(arena);
    auto values_counts = ok::List<uint32_t>::alloc(arena);

    while (true) {
        size_t values_count = 0;

        TRY(expect(Token::L_PAREN));
        while (true) {
            auto expr = expression();
            TRY(expr);

            values.push(expr.value);
            values_count++;

            if (!try_expect(Token::COMMA)) break;
        }
        TRY(expect(Token::R_PAREN));

        values_counts.push(values_count);

        if (!try_expect(Token::COMMA)) break;
    }

    return InsertStmt::alloc(arena, table_expr.value, columns.slice(), values.slice(), values_counts.slice());
}

Optional<UpdateStmt*> Parser::update_stmt() {
    TRY(expect(Token::KW_UPDATE));

    auto table_expr = expression();
    TRY(table_expr);

    TRY(expect(Token::KW_SET));

    auto columns = ok::List<ok::String>::alloc(arena);
    auto values = ok::List<Expr*>::alloc(arena);

    while (true) {
        auto column = expect(Token::IDENT);
        TRY(column);

        TRY(expect(Token::EQ));

        auto value = expression();
        TRY(value);

        columns.push(column.value.data.to_string(arena));
        values.push(value.value);

        if (!try_expect(Token::COMMA)) break;
    }

    Optional<Expr*> filter_expr{};

    if (try_expect(Token::KW_WHERE)) {
        auto filter = expression();
        TRY(filter);

        filter_expr = filter.value;
    }

    TRY(expect(Token::SEMICOLON));

    return UpdateStmt::alloc(arena, table_expr.value, columns.slice(), values.slice(), filter_expr);
}

Optional<DeleteStmt*> Parser::delete_stmt() {
    TRY(expect(Token::KW_DELETE));
    TRY(expect(Token::KW_FROM));

    auto table_expr = expression();
    TRY(table_expr);

    Optional<Expr*> filter_expr{};

    if (try_expect(Token::KW_WHERE)) {
        auto filter = expression();
        TRY(filter);
        filter_expr = filter.value;
    }

    TRY(expect(Token::SEMICOLON));

    return DeleteStmt::alloc(arena, table_expr.value, filter_expr);
}

Optional<DropStmt*> Parser::drop_stmt() {
    TRY(expect(Token::KW_DROP));

    DropStmt::Target drop_target;

    if (try_expect(Token::KW_TABLE)) drop_target = DropStmt::Target::TABLE;
    else if (try_expect(Token::KW_DATABASE))
        drop_target = DropStmt::Target::DATABASE;
    else {
        auto token = get_cur_token_or_signal_eof();
        TRY(token);

        SET_TOKEN_MISMATCH(token.value, Token::KW_TABLE, Token::KW_DATABASE);
        return {};
    }

    auto name = expect(Token::IDENT);
    TRY(name);

    TRY(expect(Token::SEMICOLON));

    return DropStmt::alloc(arena, drop_target, name.value.data.to_string(arena));
}

Optional<CreateStmt*> Parser::create_stmt() {
    TRY(expect(Token::KW_CREATE));

    CreateStmt* stmt;

    if (try_expect(Token::KW_DATABASE)) {
        auto name = expect(Token::IDENT);
        TRY(name);

        stmt = CreateDatabaseStmt::alloc(arena, name.value.data.to_string(arena));
    } else if (try_expect(Token::KW_TABLE)) {
        auto name = expect(Token::IDENT);
        TRY(name);

        auto column_names = ok::List<ok::String>::alloc(arena);
        auto column_types = ok::List<ok::String>::alloc(arena);

        TRY(expect(Token::L_PAREN));

        while (true) {
            auto column_name = expect(Token::IDENT);
            TRY(column_name);

            auto column_type = expect(Token::IDENT);
            TRY(column_type);

            column_names.push(column_name.value.data.to_string(arena));
            column_types.push(column_type.value.data.to_string(arena));

            if (!try_expect(Token::COMMA)) break;
        }

        TRY(expect(Token::R_PAREN));

        stmt = CreateTableStmt::alloc(arena, name.value.data.to_string(arena), column_names.slice(),
                                      column_types.slice());
    } else {
        auto token = get_cur_token_or_signal_eof();
        TRY(token);
        SET_TOKEN_MISMATCH(token.value, Token::KW_CREATE);
        return {};
    }

    TRY(expect(Token::SEMICOLON));

    return stmt;
}

Optional<Expr*> Parser::expression() {
    auto lhs = parse_expression_prim(this);
    TRY(lhs);

    auto token = get_cur_token_or_signal_eof();

    if (!token.has_value()) return lhs;

    switch (token.value.type) {
    case Token::EQ: {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return ExprBinaryOp::alloc(arena, ExprBinaryOp::Kind::EQ, lhs.value, rhs.value);
    }
    case Token::LT: {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return ExprBinaryOp::alloc(arena, ExprBinaryOp::Kind::LT, lhs.value, rhs.value);
    }
    case Token::GT: {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return ExprBinaryOp::alloc(arena, ExprBinaryOp::Kind::GT, lhs.value, rhs.value);
    }
    default: return lhs;
    }
}

bool Parser::cur_token_is(Token::Type token_type) const {
    return pos < tokens.count && tokens[pos].type == token_type;
}

bool Parser::try_expect(Token::Type token_type) {
    if (cur_token_is(token_type)) {
        pos++;
        return true;
    }

    return false;
}

Optional<Token> Parser::expect(Token::Type token_type) {
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

Optional<Token> Parser::get_cur_token_or_signal_eof() {
    if (is_eof()) {
        set_eof();
        return {};
    }

    return tokens[pos];
}

void Parser::set_token_mismatch(Token got, ok::Slice<Token::Type> expected) {
    OK_ASSERT(expected.count != 0);

    auto token_location = locate_token(source, got);

    String message;

    auto got_sv = token_type_to_string_view(got.type);
    if (expected.count == 1) {
        auto expected_sv = token_type_to_string_view(expected[0]);
        message = String::format(arena, "expected a token of type " OK_SV_FMT ", but got " OK_SV_FMT " instead",
                                 OK_SV_ARG(expected_sv), OK_SV_ARG(got_sv));
    } else {
        message = String::alloc(arena, "expected a token of types ");

        for (size_t i = 0; i < expected.count; i++) {
            auto expected_sv = token_type_to_string_view(expected[i]);
            message.format_append(OK_SV_FMT, OK_SV_ARG(expected_sv));
            if (i != expected.count - 1) message.append("or "_sv);
        }

        message.format_append(", but got " OK_SV_FMT " instead", OK_SV_ARG(got_sv));
    }

    error = Error{message, token_location};
}

void Parser::set_eof() {
    SourceLocation location;
    if (tokens.count > 0) location = locate_token(source, tokens[tokens.count - 1]);
    else {
        location.line = 1;
        location.column = 1;
        location.length = 0;
    }

    auto message = String::alloc(arena, "unexpected EOF");
    error = Error{message, location};
}
}; // namespace xmdb::SQL
