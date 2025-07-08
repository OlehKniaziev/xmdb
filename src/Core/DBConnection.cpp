#include "DBConnection.hpp"

namespace xmdb {
using namespace SQL;
using namespace ok::literals;

DBConnection::DBConnection(DBPool *pool, DBDescriptor *db) :
    db_pool{pool}, db{db}, ir_ctx{pool->allocator, ""_sv} {}

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
    case IRInstructionOperator_ConstInt: {
        Tuple<String, S64> operands = operands_of_ConstInt(&query->untyped, i);
        TableStream<S64> stream = TableStream<S64>::once(operands.op2);

        StringView var_name = operands.op1.view();
        DBValue var_value = DBValue::integer(stream);
        ctx->put_var(i, var_name, var_value);
        break;
    }
    case IRInstructionOperator_ConstString: {
        Tuple<String, StringView> operands = operands_of_ConstString(&query->untyped, i);
        String string_constant = operands.op2.to_string(ctx->allocator);
        TableStream<String> stream = TableStream<String>::once(string_constant);

        StringView var_name = operands.op1.view();
        DBValue var_value = DBValue::string(stream);
        ctx->put_var(i, var_name, var_value);
        break;
    }
    case IRInstructionOperator_ConstTrue: {
        String var_name = operands_of_ConstTrue(&query->untyped, i);
        TableStream<bool> stream = TableStream<bool>::once(true);
        DBValue var_value = DBValue::boolean(stream);
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_ConstFalse: {
        String var_name = operands_of_ConstFalse(&query->untyped, i);
        TableStream<bool> stream = TableStream<bool>::once(false);
        DBValue var_value = DBValue::boolean(stream);
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_ConstNull: {
        String var_name = operands_of_ConstNull(&query->untyped, i);
        TableStream<Null> stream = TableStream<Null>::once({});
        DBValue var_value = DBValue::null(stream);
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_InsertColumn: {
        Triple<U32, U32, StringView> operands = operands_of_InsertColumn(&query->untyped, i);
        DBValue table_value = ctx->fetch_var(operands.op1);
        OK_ASSERT(table_value.type == SQL::TYPE_TABLE);

        DBTable *table = table_value.u.table;
        DBValue column_value = ctx->fetch_var(operands.op2);
        StringView column_name = operands.op3;

        ctx->insert_column(table, column_name, column_value);
        break;
    }
    case IRInstructionOperator_InsertRow: {
        U32 table_ip = operands_of_InsertRow(&query->untyped, i);
        DBValue table_value = ctx->fetch_var(table_ip);
        OK_ASSERT(table_value.type == SQL::TYPE_TABLE);

        DBTable *table = table_value.u.table;
        ctx->insert_row(table);

        break;
    }
    case IRInstructionOperator_CommitInsert: { // NOTE(oleh): This instruction has no operands!
        ctx->commit_insert();
        break;
    }
    case IRInstructionOperator_UpdateColumn: {
        Triple<U32, U32, StringView> operands = operands_of_UpdateColumn(&query->untyped, i);

        DBValue table_value = ctx->fetch_var(operands.op1);
        OK_ASSERT(table_value.type == SQL::TYPE_TABLE);
        DBTable *table = table_value.u.table;

        DBValue column_value = ctx->fetch_var(operands.op2);
        StringView column_name = operands.op3;
        ctx->update_column(table, column_name, column_value);
        break;
    }
    case IRInstructionOperator_CommitUpdate: { // NOTE(oleh): This instruction has no operands!
        ctx->commit_update();
        break;
    }
    case IRInstructionOperator_DeleteTable: {
        U32 table_ip = operands_of_DeleteTable(&query->untyped, i);

        DBValue table_value = ctx->fetch_var(table_ip);
        OK_ASSERT(table_value.type == SQL::TYPE_TABLE);
        DBTable *table = table_value.u.table;

        ctx->delete_table(table);
        break;
    }
    case IRInstructionOperator_CreateDatabase: {
        StringView database_name = operands_of_CreateDatabase(&query->untyped, i);
        conn->db_pool->create_db(database_name);
        break;
    }
    case IRInstructionOperator_UseDatabase: {
        StringView database_name = operands_of_UseDatabase(&query->untyped, i);
        DBDescriptor *db = conn->db_pool->get_db(database_name);
        ctx->current_db = db;
        conn->db = db;
        break;
    }
    case IRInstructionOperator_DropTable: {
        StringView table_name = operands_of_DropTable(&query->untyped, i);
        bool deleted = false;

        for (UZ i = 0; i < conn->db->tables.count; ++i) {
            if (conn->db->tables[i]->name == table_name) {
                deleted = true;
                conn->db->tables.remove_at(i);
                break;
            }
        }

        OK_ASSERT(deleted);
        break;
    }
    case IRInstructionOperator_DropDatabase: {
        StringView database_name = operands_of_DropDatabase(&query->untyped, i);
        bool deleted = false;

        DBDescriptor *prev_db = nullptr;
        DBDescriptor *db = conn->db_pool->db_descriptors;
        // NOTE(oleh): No databases exist, signal an error. Or should this have been caught by
        // semantic analysis?
        if (db == nullptr) OK_TODO();

        while (db != nullptr) {
            if (db->name == database_name) {
                if (prev_db) {
                    prev_db->next = db->next;
                }

                if (db == conn->db_pool->db_descriptors) {
                    conn->db_pool->db_descriptors = nullptr;
                }

                deleted = true;
                break;
            }

            prev_db = db;
            db = db->next;
        }

        OK_ASSERT(deleted);
        break;
    }
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
