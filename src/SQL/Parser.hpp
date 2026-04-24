#ifndef XMDB_SQLPARSER_H_
#define XMDB_SQLPARSER_H_

#include <Core/SourceLocation.hpp>
#include <Core/util.hpp>

#include "Lexer.hpp"
#include "ast.hpp"

namespace xmdb::SQL
{
/**
 * @brief Parses SQL source code into an Abstract Syntax Tree (AST).
 */
struct Parser
{
    /**
     * @brief Constructs a new Parser.
     * @param allocator The arena allocator to use for AST nodes.
     * @param source The SQL source string to parse.
     */
    explicit Parser(ok::ArenaAllocator *allocator, StringView source);

    /**
     * @brief Parses a single SQL statement.
     * @return An optional pointer to the parsed Stmt, or empty on failure.
     */
    Optional<Stmt *> stmt();

    /**
     * @brief Parses a USE statement.
     */
    Optional<UseStmt *> use_stmt();

    /**
     * @brief Parses an INSERT statement.
     */
    Optional<InsertStmt *> insert_stmt();

    /**
     * @brief Parses an UPDATE statement.
     */
    Optional<UpdateStmt *> update_stmt();

    /**
     * @brief Parses a DELETE statement.
     */
    Optional<DeleteStmt *> delete_stmt();

    /**
     * @brief Parses a DROP statement.
     */
    Optional<DropStmt *> drop_stmt();

    /**
     * @brief Parses a CREATE statement.
     */
    Optional<CreateStmt *> create_stmt();

    /**
     * @brief Parses an ALTER statement.
     */
    Optional<AlterStmt *> alter_stmt();

    /**
     * @brief Parses a SET clause (e.g., in UPDATE).
     */
    Optional<SetClause> set_clause();

    /**
     * @brief Parses a SQL expression.
     */
    Optional<Expr *> expression();

    /**
     * @brief Parses a full SQL query (collection of statements).
     */
    Optional<Query> query();

    /**
     * @brief Checks if the current token matches a specific type.
     */
    bool cur_token_is(Token::Type type) const;

    /**
     * @brief Attempts to consume the current token if it matches a specific
     * type.
     * @return true if consumed, false otherwise.
     */
    bool try_expect(Token::Type type);

    /**
     * @brief Expects the current token to match a specific type and consumes
     * it. On failure sets the error value.
     * @return An optional containing the consumed token, or empty on failure.
     */
    Optional<Token> expect(Token::Type type);

    /**
     * @brief Gets the current token without consuming it.
     */
    Optional<Token> get_cur_token();

    /**
     * @brief Gets the current token or signals EOF error if no more tokens.
     */
    Optional<Token> get_cur_token_or_signal_eof();

    /**
     * @brief Sets a token mismatch error.
     */
    void set_token_mismatch(Token token, ok::Slice<Token::Type> types);

    /**
     * @brief Manually sets the parser state to EOF.
     */
    void set_eof();

    /**
     * @brief Checks if the parser has reached the end of the token stream.
     */
    inline bool is_eof() const
    {
        return pos >= tokens.count;
    }

    ok::ArenaAllocator *arena; ///< The allocator for AST nodes.
    StringView source; ///< The original SQL source.
    // TODO(oleh): Change this to be either some stream, or just a pointer to a
    // Lexer.
    ok::Slice<Token> tokens; ///< The tokens.
    size_t pos = 0; ///< Current position in the token stream.
    Optional<ErrorWithSourceLocation>
            error; ///< Error information if parsing failed.
};
}; // namespace xmdb::SQL

#endif // XMDB_SQLPARSER_H_
