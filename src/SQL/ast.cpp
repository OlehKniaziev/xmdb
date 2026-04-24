#include "ast.hpp"
#include <Core/ok.hpp>

using namespace ok::literals;

namespace xmdb::SQL
{
U64 ok_hash_value(const Expr &expr)
{
    U64 hash = (U64) expr.type << 32;

    switch (expr.type)
    {
    case Expr::NULL_LIT:
    case Expr::TRUE_LIT:
    case Expr::FALSE_LIT:
    case Expr::STAR:      return hash;
    case Expr::INTEGER_LIT:
    {
        auto &expr_int = static_cast<const IntegerExpr &>(expr);
        return hash | (U64) expr_int.value;
    }
    case Expr::STRING_LIT:
    {
        auto &expr_string = static_cast<const StringExpr &>(expr);
        return hash ^ ok::Hash<ok::String>::hash(expr_string.value);
    }
    case Expr::IDENT:
    {
        auto &expr_ident = static_cast<const IdentifierExpr &>(expr);
        return hash ^ ok::Hash<ok::String>::hash(expr_ident.value);
    }
    case Expr::BINARY_OP:
    {
        auto &expr_op = static_cast<const BinaryOpExpr &>(expr);
        return hash ^ ok_hash_value(*expr_op.lhs) ^ ok_hash_value(*expr_op.rhs);
    }
    case Expr::SELECT:
    {
        auto &expr_select = static_cast<const SelectExpr &>(expr);

        hash ^= ok_hash_value(*expr_select.table);

        for (UZ i = 0; i < expr_select.exprs.count; ++i)
        {
            hash ^= ok_hash_value(*expr_select.exprs[i]);
        }

        return hash;
    }
    case Expr::CALL:
    {
        auto &call = static_cast<const CallExpr &>(expr);

        hash ^= ok_hash_value(*call.fn);

        for (UZ i = 0; i < call.args.count; ++i)
        {
            Expr &arg = *call.args[i];
            hash ^= ok_hash_value(arg);
        }

        return hash;
    }
    }

    OK_UNREACHABLE();
}

bool Expr::operator==(const Expr &other) const
{
    if (type != other.type) return false;

    switch (type)
    {
    case Expr::STAR:
    case Expr::NULL_LIT:
    case Expr::TRUE_LIT:
    case Expr::FALSE_LIT: return true;
    case Expr::INTEGER_LIT:
    {
        auto *this_int = static_cast<const IntegerExpr *>(this);
        auto &other_int = static_cast<const IntegerExpr &>(other);

        return this_int->value == other_int.value;
    }
    case Expr::STRING_LIT:
    {
        auto *this_string = static_cast<const StringExpr *>(this);
        auto &other_string = static_cast<const StringExpr &>(other);

        return this_string->value == other_string.value;
    }
    case Expr::IDENT:
    {
        auto *this_ident = static_cast<const IdentifierExpr *>(this);
        auto &other_ident = static_cast<const IdentifierExpr &>(other);

        return this_ident->value == other_ident.value;
    }
    case Expr::BINARY_OP:
    {
        auto *this_op = static_cast<const BinaryOpExpr *>(this);
        auto &other_op = static_cast<const BinaryOpExpr &>(other);

        return this_op->kind == other_op.kind &&
               *this_op->lhs == *other_op.lhs && *this_op->rhs == *other_op.rhs;
    }
    case Expr::SELECT:
    {
        auto *this_select = static_cast<const SelectExpr *>(this);
        auto &other_select = static_cast<const SelectExpr &>(other);

        if (*this_select->table != *other_select.table) return false;

        if (this_select->exprs.count != other_select.exprs.count) return false;

        for (UZ i = 0; i < other_select.exprs.count; ++i)
        {
            if (this_select->exprs[i] != other_select.exprs[i]) return false;
        }

        return true;
    }
    case Expr::CALL:
    {
        auto *this_call = static_cast<const CallExpr *>(this);
        auto &other_call = static_cast<const CallExpr &>(other);

        if (this_call->fn != other_call.fn) return false;

        if (this_call->args.count != other_call.args.count) return false;

        for (UZ i = 0; i < this_call->args.count; ++i)
        {
            if (this_call->args[i] != other_call.args[i]) return false;
        }

        return true;
    }
    }

    OK_UNREACHABLE();
}

ok::String Expr::to_string(ok::Allocator *allocator) const
{
    switch (type)
    {
    case Expr::INTEGER_LIT:
    {
        auto *integer = static_cast<const IntegerExpr *>(this);
        return ok::to_string(allocator, integer->value);
    }
    case Expr::STRING_LIT:
    {
        auto *string = static_cast<const StringExpr *>(this);
        return ok::String::format(allocator, "'%s'", string->value.cstr());
    }
    case Expr::IDENT:
    {
        auto *ident = static_cast<const IdentifierExpr *>(this);
        return ident->value.copy(allocator);
    }
    case Expr::CALL:
    {
        auto *call = static_cast<const CallExpr *>(this);
        ok::String fn_string = call->fn->to_string(ok::temp_allocator());
        ok::String result =
                ok::String::format(allocator, "%s(", fn_string.cstr());

        if (call->args.count == 0)
        {
            goto end;
        }

        for (SZ i = 0; i < (SZ) call->args.count - 1; ++i)
        {
            Expr *arg = call->args[i];
            ok::String arg_string = arg->to_string(ok::temp_allocator());
            result.append(arg_string);
            result.append(", "_sv);
        }

        {
            Expr *last_arg = call->args[call->args.count - 1];
            ok::String arg_string = last_arg->to_string(ok::temp_allocator());
            result.append(arg_string);
        }

    end:
        result.push(')');
        return result;
    }
    case Expr::TRUE_LIT:  return ok::String::alloc(allocator, "TRUE");
    case Expr::FALSE_LIT: return ok::String::alloc(allocator, "FALSE");
    case Expr::NULL_LIT:  return ok::String::alloc(allocator, "NULL");
    case Expr::STAR:      return ok::String::alloc(allocator, "*");
    case Expr::BINARY_OP: OK_TODO();
    case Expr::SELECT:    OK_TODO();
    }

    OK_UNREACHABLE();
}
}; // namespace xmdb::SQL
