#include "type_check2.hpp"

namespace xmdb::SQL
{
Result<TypedQuery, ErrorWithSourceLocation> TypingContext::type_check_query(Query *query)
{
    for (UZ i = 0; i < query->stmts.count; ++i)
    {
        Stmt *stmt = query->stmts[i];
        switch (stmt->type)
        {

        }
    }
}
}
