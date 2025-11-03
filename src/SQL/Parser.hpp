#ifndef XMDB_SQLPARSER_H_
#define XMDB_SQLPARSER_H_

#include <Core/SourceLocation.hpp>
#include <Core/util.hpp>

#include "Lexer.hpp"
#include "ast.hpp"

namespace xmdb::SQL {
struct Parser {
    explicit Parser(ok::ArenaAllocator*, StringView);

    Optional<Stmt*> stmt();
    Optional<UseStmt*> use_stmt();
    Optional<InsertStmt*> insert_stmt();
    Optional<UpdateStmt*> update_stmt();
    Optional<DeleteStmt*> delete_stmt();
    Optional<DropStmt*> drop_stmt();
    Optional<CreateStmt*> create_stmt();
    Optional<AlterStmt*> alter_stmt();

    Optional<SetClause> set_clause();

    Optional<Expr*> expression();

    Optional<Query> query();

    bool cur_token_is(Token::Type) const;
    bool try_expect(Token::Type);
    Optional<Token> expect(Token::Type);

    Optional<Token> get_cur_token_or_signal_eof();

    void set_token_mismatch(Token, ok::Slice<Token::Type>);

    void set_eof();

    inline bool is_eof() const {
        return pos >= tokens.count;
    }

    ok::ArenaAllocator* arena;
    StringView source;
    ok::Slice<Token> tokens{};
    Optional<ErrorWithSourceLocation> error{};
    size_t pos = 0;
};
}; // namespace xmdb

#endif // XMDB_SQLPARSER_H_
