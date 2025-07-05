#include "DBConnection.hpp"

namespace xmdb {
using namespace SQL;

DBConnection::DBConnection(DBPool *pool, DBDescriptor *db) : db_pool{pool}, db{db} {}

static inline ColumnType type_to_column_type(Type type) {
    switch (type) {
    case TYPE_INT:    return COLUMN_INTEGER;
    case TYPE_STRING: return COLUMN_TEXT;
    case TYPE_IMAGE:  return COLUMN_IMAGE;
    case TYPE_BOOL:   return COLUMN_BOOLEAN;
    case TYPE_FLOAT:  return COLUMN_FLOAT;
    case TYPE_DOUBLE: return COLUMN_DOUBLE;
    case TYPE_NULL:
    case TYPE_TABLE: OK_PANIC("Unsupported type to column type conversion");
    case TYPE_MAX: OK_UNREACHABLE();
    }

    OK_UNREACHABLE();
}

static void execute_instruction(TypedCompiledQuery *query, UZ i, QueryExecutionContext *ctx, DBConnection *conn) {
    IRInstruction instr = query->untyped.instructions[i];
    switch (instr.op) {
    case IRInstructionOperator_Eq: {
        Triple<String, U32, U32> operands = operands_of_Eq(&query->untyped, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Gt: {
        Triple<String, U32, U32> operands = operands_of_Eq(&query->untyped, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Lt: {
        Triple<String, U32, U32> operands = operands_of_Eq(&query->untyped, i);

        DBValue lhs = ctx->fetch_var(operands.op2);
        DBValue rhs = ctx->fetch_var(operands.op3);
        DBValue var_value = lhs.cmp(conn->db_pool->allocator, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_FetchColumn: {
        Triple<String, U32, StringView> operands = operands_of_FetchColumn(&query->untyped, i);

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
        Triple<String, StringView, TableSchema *> operands = operands_of_FetchTable(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op2);
        DBValue var_value = DBValue::table(table);
        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_CreateTable: {
        Tuple<StringView, TableSchema *> operands = operands_of_CreateTable(&query->untyped, i);
        ctx->create_table(operands.op1, operands.op2);
        break;
    }
    case IRInstructionOperator_EmitColumn: {
        Tuple<U32, StringView> operands = operands_of_EmitColumn(&query->untyped, i);
        DBValue column_value = ctx->fetch_var(operands.op1);
        StringView column_name = operands.op2;
        ctx->emit_column(column_value, column_name);
        break;
    }
    case IRInstructionOperator_EmitQuery: {
        Tuple<String, U32> operands = operands_of_EmitQuery(&query->untyped, i);
        Optional<TypedTableSchema> opt_query_schema = query->table_types.get(i);
        TypedTableSchema query_schema = opt_query_schema.get();

        UZ column_count = query_schema.column_types.count;
        ColumnType *column_types = ctx->allocator->alloc<ColumnType>(column_count);
        for (UZ i = 0; i < column_count; ++i) {
            Type type = query_schema.column_types[i];
            ColumnType column_type = type_to_column_type(type);
            column_types[i] = column_type;
        }

        DBTable *emitted_query = ctx->emit_query(operands.op2, column_types);
        DBValue var_value = DBValue::table(emitted_query);
        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);
        break;
    }
    default: OK_TODO();
    }
}

void DBConnection::execute(TypedCompiledQuery *query, QueryResults *results) {
    QueryExecutionContext *ctx = db_pool->rent_empty_execution_context(db);

    UZ instructions_count = query->untyped.instructions.count;

    for (UZ i = 0; i < instructions_count; ++i) {
        execute_instruction(query, i, ctx, this);
    }

    // FIXME(oleh): This should depend on the result of executing query instructions.
    results->ok = true;

    if (ctx->last_emitted_query.has_value()) {
        results->value = ctx->last_emitted_query.value;
    }

    db_pool->return_execution_context(ctx);
}
}; // namespace xmdb
