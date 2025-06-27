#include "DBConnection.hpp"

namespace xmdb {
using namespace SQL;

DBConnection::DBConnection(DBPool *pool) : db_pool{pool} {}

static void execute_instruction(CompiledQuery *query, UZ i, QueryExecutionContext *ctx, DBConnection *conn) {
    IRInstruction instr = query->instructions[i];
    switch (instr.op) {
    case IRInstructionOperator_Eq: {
        Triple<String, U32, U32> operands = operands_of_Eq(query, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Gt: {
        Triple<String, U32, U32> operands = operands_of_Eq(query, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Lt: {
        Triple<String, U32, U32> operands = operands_of_Eq(query, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_FetchColumn: {
        Triple<String, U32, StringView> operands = operands_of_FetchColumn(query, i);

        DBValue table_val = ctx->fetch_var(operands.op2);
        OK_ASSERT(table_val.type == Type::TYPE_TABLE);

        DBTable *table = table_val.u.table;
        StringView column_name = operands.op3;
        DBValue column = ctx->fetch_column(table, column_name);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, column);

        break;
    }
    case IRInstructionOperator_FetchTable: {
        Triple<String, StringView, TableSchema *> operands = operands_of_FetchTable(query, i);

        DBTable *table = ctx->fetch_table(operands.op2);
        DBValue var_value = DBValue::table(table);
        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_CreateTable: {
        Tuple<StringView, TableSchema *> operands = operands_of_CreateTable(query, i);
        ctx->create_table(operands.op1, operands.op2);
        break;
    }
    case IRInstructionOperator_EmitColumn: {
        Tuple<U32, StringView> operands = operands_of_EmitColumn(query, i);
        DBValue column_value = ctx->fetch_var(operands.op1);
        StringView column_name = operands.op2;
        ctx->emit_column(column_value, column_name);
        break;
    }
    case IRInstructionOperator_EmitQuery: {
        Tuple<String, U32> operands = operands_of_EmitQuery(query, i);
        DBTable *emitted_query = ctx->emit_query(operands.op2);
        DBValue db_value = DBValue::table(emitted_query);
        OK_TODO();
        ctx->put_var();
        break;
    }
    default: OK_TODO();
    }
}

void DBConnection::execute(CompiledQuery *query) {
    QueryExecutionContext *ctx = db_pool->rent_empty_execution_context();

    for (UZ i = 0; i < query->instructions.count; ++i) {
        execute_instruction(query, i, ctx, this);
    }

    db_pool->return_execution_context(ctx);
}
}; // namespace xmdb
