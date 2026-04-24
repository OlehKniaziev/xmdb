#include "Parser.hpp"
#include <Core/ok.hpp>
#include <Core/util.hpp>
#include <SQL/ast.hpp>

#define SET_TOKEN_MISMATCH(p, got, ...)                                        \
    do {                                                                       \
        Token::Type expected_arr[] = {__VA_ARGS__};                            \
        ::ok::Slice<Token::Type> expected = {expected_arr,                     \
                                             OK_ARR_LEN(expected_arr)};        \
        p->set_token_mismatch((got), expected);                                \
    }                                                                          \
    while (0)

using namespace ok::literals;

namespace xmdb::SQL
{
Optional<SelectExpr *> parse_select_expr(Parser *p)
{
    Optional<Token> select_token = p->expect(Token::KW_SELECT);
    TRY(select_token);

    auto exprs = ok::List<Expr *>::alloc(p->arena);

    while (true)
    {
        auto expr = p->expression();
        TRY(expr);

        exprs.push(expr.value);

        if (!p->try_expect(Token::COMMA)) break;
    }

    TRY(p->expect(Token::KW_FROM));

    auto table_expr = p->expression();
    TRY(table_expr);

    TRY(p->expect(Token::SEMICOLON));

    return SelectExpr::alloc(p->arena, select_token.value, exprs.slice(),
                             table_expr.value);
}

Optional<Expr *> parse_expression_prim(Parser *parser)
{
    auto token = parser->get_cur_token_or_signal_eof();
    if (!token.has_value()) return {};

    switch (token.value.type)
    {
    case Token::IDENT:
    {
        ++parser->pos;
        return IdentifierExpr::alloc(parser->arena, token.value,
                                     token.value.data);
    }
    case Token::INTEGER:
    {
        ++parser->pos;
        int64_t integer;
        OK_ASSERT(ok::parse_int64(token.value.data, &integer));
        return IntegerExpr::alloc(parser->arena, token.value, integer);
    }
    case Token::STRING:
    {
        ++parser->pos;
        String value = token.value.data.to_string(parser->arena);
        return StringExpr::alloc(parser->arena, token.value, value);
    }
    case Token::STAR:
    {
        ++parser->pos;
        return StarExpr::alloc(parser->arena, token.value);
    }
    case Token::KW_TRUE:
    {
        ++parser->pos;
        return Expr::alloc_true(parser->arena, token.value);
    }
    case Token::KW_FALSE:
    {
        ++parser->pos;
        return Expr::alloc_false(parser->arena, token.value);
    }
    case Token::KW_NULL:
    {
        ++parser->pos;
        return Expr::alloc_null(parser->arena, token.value);
    }
    case Token::KW_SELECT: return parse_select_expr(parser).upcast<Expr>();
    default:
    {
        SET_TOKEN_MISMATCH(parser, token.value, Token::IDENT, Token::INTEGER,
                           Token::STRING, Token::KW_TRUE, Token::KW_FALSE,
                           Token::KW_NULL, Token::KW_SELECT);
        return {};
    }
    }
}

Parser::Parser(ok::ArenaAllocator *arena, StringView source) :
    arena{arena}, source{source}
{
    auto tokens = ok::List<Token>::alloc(arena);
    Lexer lexer{source};

    for (auto token = lexer.next(); token.has_value(); token = lexer.next())
    {
        tokens.push(token.value);
    }

    this->tokens = tokens.slice();
}

Optional<Stmt *> Parser::stmt()
{
    auto cur_token = get_cur_token_or_signal_eof();
    TRY(cur_token);

    switch (cur_token.value.type)
    {
    case Token::KW_DELETE: return delete_stmt().upcast<Stmt>();
    case Token::KW_INSERT: return insert_stmt().upcast<Stmt>();
    case Token::KW_UPDATE: return update_stmt().upcast<Stmt>();
    case Token::KW_DROP:   return drop_stmt().upcast<Stmt>();
    case Token::KW_CREATE: return create_stmt().upcast<Stmt>();
    case Token::KW_USE:    return use_stmt().upcast<Stmt>();
    case Token::KW_ALTER:  return alter_stmt().upcast<Stmt>();
    default:
    {
        auto expr = expression();
        TRY(expr);
        return ExprStmt::alloc(arena, expr.value->token, expr.value);
    }
    }
}

Optional<UseStmt *> Parser::use_stmt()
{
    Optional<Token> use_token = expect(Token::KW_USE);
    TRY(use_token);

    auto database = expect(Token::IDENT);
    TRY(database);

    TRY(expect(Token::SEMICOLON));

    return UseStmt::alloc(arena, use_token.value, database.value.data);
}

Optional<InsertStmt *> Parser::insert_stmt()
{
    Optional<Token> insert_token = expect(Token::KW_INSERT);
    TRY(insert_token);
    TRY(expect(Token::KW_INTO));

    auto table_name = expect(Token::IDENT);

    TRY(expect(Token::L_PAREN));

    auto columns = ok::List<ok::String>::alloc(arena);

    while (true)
    {
        auto column = expect(Token::IDENT);
        TRY(column);
        columns.push(column.value.data.to_string(arena));

        if (!try_expect(Token::COMMA)) break;
    }

    TRY(expect(Token::R_PAREN));

    TRY(expect(Token::KW_VALUES));

    auto values = ok::List<Expr *>::alloc(arena);
    auto values_counts = ok::List<uint32_t>::alloc(arena);

    while (true)
    {
        size_t values_count = 0;

        TRY(expect(Token::L_PAREN));
        while (true)
        {
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

    TRY(expect(Token::SEMICOLON));

    return InsertStmt::alloc(arena, insert_token.value, table_name.value.data,
                             columns.slice(), values.slice(),
                             values_counts.slice());
}

Optional<UpdateStmt *> Parser::update_stmt()
{
    Optional<Token> update_token = expect(Token::KW_UPDATE);
    TRY(update_token);

    auto table_expr = expression();
    TRY(table_expr);

    TRY(expect(Token::KW_SET));

    auto columns = ok::List<ok::String>::alloc(arena);
    auto values = ok::List<Expr *>::alloc(arena);

    while (true)
    {
        auto column = expect(Token::IDENT);
        TRY(column);

        TRY(expect(Token::EQ));

        auto value = expression();
        TRY(value);

        columns.push(column.value.data.to_string(arena));
        values.push(value.value);

        if (!try_expect(Token::COMMA)) break;
    }

    Optional<Expr *> filter_expr{};

    if (try_expect(Token::KW_WHERE))
    {
        auto filter = expression();
        TRY(filter);

        filter_expr = filter.value;
    }

    TRY(expect(Token::SEMICOLON));

    return UpdateStmt::alloc(arena, update_token.value, table_expr.value,
                             columns.slice(), values.slice(), filter_expr);
}

Optional<DeleteStmt *> Parser::delete_stmt()
{
    Optional<Token> delete_token = expect(Token::KW_DELETE);
    TRY(delete_token);
    TRY(expect(Token::KW_FROM));

    auto table_expr = expression();
    TRY(table_expr);

    Optional<Expr *> filter_expr{};

    if (try_expect(Token::KW_WHERE))
    {
        auto filter = expression();
        TRY(filter);
        filter_expr = filter.value;
    }

    TRY(expect(Token::SEMICOLON));

    return DeleteStmt::alloc(arena, delete_token.value, table_expr.value,
                             filter_expr);
}

Optional<DropStmt *> Parser::drop_stmt()
{
    Optional<Token> drop_token = expect(Token::KW_DROP);
    TRY(drop_token);

    DropStmt::Target drop_target;

    if (try_expect(Token::KW_TABLE)) drop_target = DropStmt::Target::TABLE;
    else if (try_expect(Token::KW_DATABASE))
        drop_target = DropStmt::Target::DATABASE;
    else
    {
        auto token = get_cur_token_or_signal_eof();
        TRY(token);

        SET_TOKEN_MISMATCH(this, token.value, Token::KW_TABLE,
                           Token::KW_DATABASE);
        return {};
    }

    auto name = expect(Token::IDENT);
    TRY(name);

    TRY(expect(Token::SEMICOLON));

    return DropStmt::alloc(arena, drop_token.value, drop_target,
                           name.value.data.to_string(arena));
}

Optional<CreateStmt *> Parser::create_stmt()
{
    Optional<Token> create_token = expect(Token::KW_CREATE);

    CreateStmt *stmt;

    if (try_expect(Token::KW_DATABASE))
    {
        auto name = expect(Token::IDENT);
        TRY(name);

        stmt = CreateDatabaseStmt::alloc(arena, create_token.value,
                                         name.value.data.to_string(arena));
    }
    else if (try_expect(Token::KW_TABLE))
    {
        auto name = expect(Token::IDENT);
        TRY(name);

        auto column_names = ok::List<ok::String>::alloc(arena);
        auto column_types = ok::List<ok::String>::alloc(arena);

        TRY(expect(Token::L_PAREN));

        while (true)
        {
            auto column_name = expect(Token::IDENT);
            TRY(column_name);

            auto column_type = expect(Token::IDENT);
            TRY(column_type);

            column_names.push(column_name.value.data.to_string(arena));
            column_types.push(column_type.value.data.to_string(arena));

            if (!try_expect(Token::COMMA)) break;
        }

        TRY(expect(Token::R_PAREN));

        stmt = CreateTableStmt::alloc(
                arena, create_token.value, name.value.data.to_string(arena),
                column_names.slice(), column_types.slice());
    }
    else if (try_expect(Token::KW_USER))
    {
        auto name = expect(Token::IDENT);
        TRY(name);

        stmt = CreateUserStmt::alloc(arena, create_token.value,
                                     name.value.data.to_string(arena));
    }
    else
    {
        auto token = get_cur_token_or_signal_eof();
        TRY(token);
        SET_TOKEN_MISMATCH(this, token.value, Token::KW_CREATE);
        return {};
    }

    TRY(expect(Token::SEMICOLON));

    return stmt;
}

Optional<SetClause> Parser::set_clause()
{
    auto name_token = expect(Token::IDENT);
    TRY(name_token);

    TRY(expect(Token::EQ));

    auto opt_value = expression();
    TRY(opt_value);

    return SetClause{
            .name = name_token.value.data.to_string(arena),
            .value = opt_value.value,
    };
}

Optional<AlterStmt *> Parser::alter_stmt()
{
    auto token = expect(Token::KW_ALTER);
    TRY(token);

    auto target_token = get_cur_token_or_signal_eof();
    TRY(target_token);

    auto clauses = ok::List<SetClause>::alloc(arena);

    switch (target_token.value.type)
    {
    case Token::KW_USER:
    {
        ++pos;

        auto name_token = expect(Token::IDENT);
        TRY(name_token);

        TRY(expect(Token::KW_SET));

        auto clause = set_clause();
        TRY(clause);

        clauses.push(clause.value);

        while (try_expect(Token::COMMA))
        {
            clause = set_clause();
            TRY(clause);
            clauses.push(clause.value);
        }

        TRY(expect(Token::SEMICOLON));

        return AlterUserStmt::alloc(arena, token.value, name_token.value.data,
                                    clauses.slice());
    }
    default: SET_TOKEN_MISMATCH(this, target_token.value, Token::KW_USER);
    }

    OK_UNREACHABLE();
}

Optional<Expr *> Parser::expression()
{
    auto lhs = parse_expression_prim(this);
    TRY(lhs);

    auto token = get_cur_token();

    if (!token.has_value()) return lhs;

    switch (token.value.type)
    {
    case Token::EQ:
    {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return BinaryOpExpr::alloc(arena, token.value, BinaryOpExpr::Kind::EQ,
                                   lhs.value, rhs.value);
    }
    case Token::LT:
    {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return BinaryOpExpr::alloc(arena, token.value, BinaryOpExpr::Kind::LT,
                                   lhs.value, rhs.value);
    }
    case Token::GT:
    {
        ++pos;
        auto rhs = expression();
        TRY(rhs);

        return BinaryOpExpr::alloc(arena, token.value, BinaryOpExpr::Kind::GT,
                                   lhs.value, rhs.value);
    }
    case Token::L_PAREN:
    {
        ++pos;

        ok::List<Expr *> args = ok::List<Expr *>::alloc(arena);

        while (true)
        {
            auto r_paren = get_cur_token_or_signal_eof();
            TRY(r_paren);

            if (r_paren.get().type == Token::R_PAREN)
            {
                ++pos;
                return CallExpr::alloc(arena, token.value, lhs.value,
                                       args.slice());
            }

            auto expr = expression();
            TRY(expr);

            args.push(expr.get());

            auto maybe_comma = get_cur_token_or_signal_eof();
            TRY(maybe_comma);

            if (maybe_comma.get().type == Token::COMMA)
            {
                ++pos;
            }
        }
    }
    default: return lhs;
    }
}

Optional<Query> Parser::query()
{
    auto stmts = ok::List<Stmt *>::alloc(arena);
    while (!is_eof())
    {
        auto stmt_opt = stmt();
        TRY(stmt_opt);
        stmts.push(stmt_opt.value);
    }
    return Query{stmts.slice()};
}

bool Parser::cur_token_is(Token::Type token_type) const
{
    return pos < tokens.count && tokens[pos].type == token_type;
}

bool Parser::try_expect(Token::Type token_type)
{
    if (cur_token_is(token_type))
    {
        pos++;
        return true;
    }

    return false;
}

Optional<Token> Parser::expect(Token::Type token_type)
{
    if (is_eof())
    {
        set_eof();
        return {};
    }

    if (cur_token_is(token_type))
    {
        return tokens[pos++];
    }

    SET_TOKEN_MISMATCH(this, tokens[pos], token_type);

    return {};
}

Optional<Token> Parser::get_cur_token()
{
    if (is_eof())
    {
        return {};
    }
    return tokens[pos];
}

Optional<Token> Parser::get_cur_token_or_signal_eof()
{
    auto token = get_cur_token();
    if (!token)
    {
        set_eof();
    }
    return token;
}

void Parser::set_token_mismatch(Token got, ok::Slice<Token::Type> expected)
{
    OK_ASSERT(expected.count != 0);

    auto token_location = locate_token(source, got);

    String message;

    auto got_sv = token_type_to_string_view(got.type);
    if (expected.count == 1)
    {
        auto expected_sv = token_type_to_string_view(expected[0]);
        message = String::format(arena,
                                 "expected a token of type " OK_SV_FMT
                                 ", but got " OK_SV_FMT " instead",
                                 OK_SV_ARG(expected_sv), OK_SV_ARG(got_sv));
    }
    else
    {
        message = String::alloc(arena, "expected a token of types ");

        for (size_t i = 0; i < expected.count; i++)
        {
            auto expected_sv = token_type_to_string_view(expected[i]);
            message.format_append(OK_SV_FMT, OK_SV_ARG(expected_sv));
            if (i != expected.count - 1) message.append(" or "_sv);
        }

        message.format_append(", but got " OK_SV_FMT " instead",
                              OK_SV_ARG(got_sv));
    }

    error = ErrorWithSourceLocation{message, token_location};
}

void Parser::set_eof()
{
    SourceLocation location;
    if (tokens.count > 0)
        location = locate_token(source, tokens[tokens.count - 1]);
    else
    {
        location.line = 1;
        location.column = 1;
        location.length = 0;
    }

    auto message = String::alloc(arena, "unexpected EOF");
    error = ErrorWithSourceLocation{message, location};
}
}; // namespace xmdb::SQL
