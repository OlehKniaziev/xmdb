#include "ast.hpp"

namespace xmdb::SQL {
Expr true_literal_expr{Expr::TRUE_LIT};
Expr false_literal_expr{Expr::FALSE_LIT};
Expr null_literal_expr{Expr::NULL_LIT};

Expr* Expr::true_literal = &true_literal_expr;
Expr* Expr::false_literal = &false_literal_expr;
Expr* Expr::null_literal = &null_literal_expr;
};