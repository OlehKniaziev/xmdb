#include "SQLLexer.hpp"

using namespace ok::literals;

namespace xmdb {
__attribute__((constructor)) static void global_state_ctor() {
    sql_token_table = Table<StringView, SQLToken::Type>::alloc(ok::static_allocator);
#define X(str, kw) sql_token_table.put(StringView{str}, SQLToken::kw);
    XMDB_ENUM_SQL_KEYWORDS
    #undef X
}
};