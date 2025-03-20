#ifndef XMDB_SQLPARSER_H_
#define XMDB_SQLPARSER_H_

#include "SQLLexer.hpp"
#include "ast.hpp"

namespace xmdb {
struct SQLParser {
    explicit SQLParser(ok::ArenaAllocator*, StringView);

    Optional<SQLSelectStmt*> select_stmt();
    Optional<SQLExpr*> expression();

    bool cur_token_is(SQLToken::Type) const;
    bool try_expect(SQLToken::Type);
    Optional<SQLToken> expect(SQLToken::Type);

    Optional<SQLToken> get_cur_token_or_signal_eof() const;

    ok::ArenaAllocator* arena;
    ok::Slice<SQLToken> tokens{};
    Optional<String> error{};
    size_t pos = 0;
};
}; // namespace xmdb

#endif // XMDB_SQLPARSER_H_
