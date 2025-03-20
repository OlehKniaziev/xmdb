#ifndef XMDB_SQLPARSER_H_
#define XMDB_SQLPARSER_H_

#include "SQLLexer.hpp"
#include "SourceLocation.hpp"
#include "ast.hpp"

namespace xmdb {
struct SQLParser {
    struct Error {
        String message;
        SourceLocation location;
    };

    explicit SQLParser(ok::ArenaAllocator*, StringView);

    Optional<SQLSelectStmt*> select_stmt();
    Optional<SQLUseStmt*> use_stmt();
    Optional<SQLInsertStmt*> insert_stmt();

    Optional<SQLExpr*> expression();

    bool cur_token_is(SQLToken::Type) const;
    bool try_expect(SQLToken::Type);
    Optional<SQLToken> expect(SQLToken::Type);

    Optional<SQLToken> get_cur_token_or_signal_eof();

    void set_token_mismatch(SQLToken, ok::Slice<SQLToken::Type>);

    void set_eof();

    inline bool is_eof() const {
        return pos >= tokens.count;
    }

    ok::ArenaAllocator* arena;
    StringView source;
    ok::Slice<SQLToken> tokens{};
    Optional<Error> error{};
    size_t pos = 0;
};
}; // namespace xmdb

#endif // XMDB_SQLPARSER_H_
