#include "Lexer.hpp"

using namespace ok::literals;

namespace xmdb {
__attribute__((constructor)) static void global_state_ctor() {
    SQL::token_table = Table<StringView, SQL::Token::Type>::alloc(ok::static_allocator);
#define X(str, kw) SQL::token_table.put(StringView{str}, SQL::Token::kw);
    XMDB_ENUM_SQL_KEYWORDS
    #undef X
}
};