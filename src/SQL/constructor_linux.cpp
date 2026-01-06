#include "Lexer.hpp"

using namespace ok::literals;

namespace xmdb::SQL {
__attribute__((constructor)) void init_global_state() {
    ok::ArenaAllocator state_arena{};

    SQL::token_table = Table<StringView, SQL::Token::Type>::alloc(&state_arena);
#define X(str, kw) SQL::token_table.put(StringView{str}, SQL::Token::kw);
    XMDB_ENUM_SQL_KEYWORDS
#undef X
}
};
