#include "DBConnection.hpp"
#include "Logger.hpp"

#include <SQL/ir.hpp>
#include <csetjmp>

namespace xmdb {
using namespace SQL;
using namespace ok::literals;

DBConnection::DBConnection(DBPool *pool, DBDescriptor *db, DBUser *user) :
    db_pool{pool}, user{user}, db{db}, ir_ctx{pool->allocator, ""_sv} {
}

static inline ColumnType type_to_column_type(Type type) {
    switch (type) {
    case TYPE_INT:    return ColumnType::INTEGER;
    case TYPE_STRING: return ColumnType::TEXT;
    case TYPE_IMAGE:  return ColumnType::IMAGE;
    case TYPE_BOOL:   return ColumnType::BOOLEAN;
    case TYPE_FLOAT:  return ColumnType::FLOAT;
    case TYPE_DOUBLE: return ColumnType::DOUBLE;
    case TYPE_NULL:
    case TYPE_TABLE:  OK_PANIC("Unsupported type to column type conversion");
    }

    OK_UNREACHABLE();
}

static void execute_instruction(TypedCompiledQuery *query, UZ i, QueryExecutionContext *ctx, DBConnection *conn) {
    IRInstruction instr = query->untyped.instructions[i];
    switch (instr.op) {
    case IRInstructionOperator_Eq: {
        Triple<String, U32, U32> operands = operands_of_Eq(&query->untyped, i);

        DBValue *lhs = ctx->fetch_var(operands.op2);
        DBValue *rhs = ctx->fetch_var(operands.op3);
        DBValue *var_value = ctx->compare(lhs, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Gt: {
        Triple<String, U32, U32> operands = operands_of_Gt(&query->untyped, i);

        DBValue *lhs = ctx->fetch_var(operands.op2);
        DBValue *rhs = ctx->fetch_var(operands.op3);
        DBValue *var_value = ctx->compare(lhs, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_Lt: {
        Triple<String, U32, U32> operands = operands_of_Lt(&query->untyped, i);

        DBValue *lhs = ctx->fetch_var(operands.op2);
        DBValue *rhs = ctx->fetch_var(operands.op3);
        DBValue *var_value = ctx->compare(lhs, rhs);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, var_value);

        break;
    }
    case IRInstructionOperator_FetchColumn: {
        Triple<String, U32, StringView> operands = operands_of_FetchColumn(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op2);

        StringView column_name = operands.op3;
        DBValue *column = ctx->fetch_column(table, column_name);

        StringView var_name = operands.op1.view();
        ctx->put_var(i, var_name, column);

        break;
    }
    case IRInstructionOperator_FetchTable: {
        Triple<String, StringView, TableSchema *> operands = operands_of_FetchTable(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op2);
        ctx->put_table(i, table);

        break;
    }
    case IRInstructionOperator_CreateTable: {
        Tuple<StringView, TableSchema *> operands = operands_of_CreateTable(&query->untyped, i);
        ctx->create_table(operands.op1, operands.op2);
        break;
    }
    case IRInstructionOperator_CreateUser: {
        StringView user_name = operands_of_CreateUser(&query->untyped, i);
        ctx->create_user(user_name);
        break;
    }
    case IRInstructionOperator_AlterUserProperty: {
        Triple<StringView, StringView, U32> operands = operands_of_AlterUserProperty(&query->untyped, i);
        DBValue *property_value = ctx->fetch_var(operands.op3);
        ctx->alter_user_property(operands.op1, operands.op2, property_value);
        break;
    }
    case IRInstructionOperator_CommitAlterUser: {
        ctx->commit_alter_user();
        break;
    }
    case IRInstructionOperator_EmitColumn: {
        Triple<U32, U32, StringView> operands = operands_of_EmitColumn(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op1);

        DBValue *column_value = ctx->fetch_var(operands.op2);
        StringView column_name = operands.op3;
        ctx->emit_column(table, column_value, column_name);
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
        ctx->put_table(i, emitted_query);
        break;
    }
    case IRInstructionOperator_ConstInt: {
        Tuple<String, S64> operands = operands_of_ConstInt(&query->untyped, i);

        StringView var_name = operands.op1.view();
        DBValue *var_value = new (ctx->allocator) ConstDBValue{operands.op2};
        ctx->put_var(i, var_name, var_value);
        break;
    }
    case IRInstructionOperator_ConstString: {
        Tuple<String, StringView> operands = operands_of_ConstString(&query->untyped, i);
        String string_constant_value = operands.op2.to_string(ctx->allocator);
        String *string_constant = ctx->allocator->alloc<String>();
        *string_constant = string_constant_value;

        StringView var_name = operands.op1.view();
        DBValue *var_value = new (ctx->allocator) ConstDBValue{string_constant};
        ctx->put_var(i, var_name, var_value);
        break;
    }
    case IRInstructionOperator_ConstTrue: {
        String var_name = operands_of_ConstTrue(&query->untyped, i);
        DBValue *var_value = new (ctx->allocator) ConstDBValue{true};
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_ConstFalse: {
        String var_name = operands_of_ConstFalse(&query->untyped, i);
        DBValue *var_value = new (ctx->allocator) ConstDBValue{false};
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_ConstNull: {
        String var_name = operands_of_ConstNull(&query->untyped, i);
        DBValue *var_value = DBValue::null();
        ctx->put_var(i, var_name.view(), var_value);
        break;
    }
    case IRInstructionOperator_InsertColumn: {
        Triple<U32, U32, StringView> operands = operands_of_InsertColumn(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op1);
        DBValue *column_value = ctx->fetch_var(operands.op2);
        StringView column_name = operands.op3;

        ctx->insert_column(table, column_name, column_value);
        break;
    }
    case IRInstructionOperator_InsertRow: {
        // TODO(oleh): Make this instruction not carry any operands
        U32 table_ip = operands_of_InsertRow(&query->untyped, i);

        DBTable *table = ctx->fetch_table(table_ip);
        OK_UNUSED(table);
        ctx->insert_row();

        break;
    }
    case IRInstructionOperator_CommitInsert: { // NOTE(oleh): This instruction has no operands!
        ctx->commit_insert();
        break;
    }
    case IRInstructionOperator_UpdateColumn: {
        Triple<U32, U32, StringView> operands = operands_of_UpdateColumn(&query->untyped, i);

        DBTable *table = ctx->fetch_table(operands.op1);

        DBValue *column_value = ctx->fetch_var(operands.op2);
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

        DBTable *table = ctx->fetch_table(table_ip);
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
            if (conn->db->tables[i]->name() == table_name) {
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

static DBValue *eval_node(QueryExecutionContext *ctx, QueryGraph::Node *node) {
    OK_UNUSED(ctx);

    switch (node->type()) {
    case QueryGraph::Node::Type::VALUE: {
        QueryGraph::ValueNode *value_node = static_cast<QueryGraph::ValueNode *>(node);
        return value_node->value();
    }
    case QueryGraph::Node::Type::ATOMIC:
        OK_TODO();
    case QueryGraph::Node::Type::READ:
    case QueryGraph::Node::Type::WRITE:
    case QueryGraph::Node::Type::ALTER_USER:
        OK_UNREACHABLE();
    }

    OK_UNREACHABLE();
}

static void run_single_node(QueryExecutionContext *ctx, QueryGraph::Node *node) {
    switch (node->type()) {
    case QueryGraph::Node::Type::ALTER_USER: {
        QueryGraph::AlterUserNode *alter_node = static_cast<QueryGraph::AlterUserNode *>(node);

        ok::StringView user_name = alter_node->user_name();

        ok::Optional<DBUser *> user_opt = ctx->current_db->find_user(user_name);

        if (!user_opt) {
            XMDB_FAIL_FMT(ctx, "Could not find requested user with name '" OK_SV_FMT "'", OK_SV_ARG(user_name));
        }

        DBUser *user = user_opt.get();

        switch (alter_node->property()) {
        case QueryGraph::AlterUserNode::Property::PASSWORD: {
            QueryGraph::Node *password_db_node = alter_node->property_value();
            DBValue *password_db_value = eval_node(ctx, password_db_node);
            DBTableStream password_value_stream = DBTableStream::from_value(ctx->allocator, password_db_value);

            Value password_value = password_value_stream.next().get();
            FixedString new_password = password_value.as_string();
            user->sha256_password_digest = sha256_digest(view(ok::temp_allocator(), new_password));

            break;
        }
        }

        break;
    }
    case QueryGraph::Node::Type::READ:
        OK_TODO_MSG("READ");
    case QueryGraph::Node::Type::WRITE:
        OK_TODO_MSG("WRITE");
    case QueryGraph::Node::Type::ATOMIC: {
        XMDB_FIXME("This is not atomic at all bruh");

        QueryGraph::AtomicNode *atomic_node = static_cast<QueryGraph::AtomicNode *>(node);

        ok::Slice<QueryGraph::Node *> child_nodes = atomic_node->nodes();

        for (UZ i = 0; i < child_nodes.count; ++i) {
            QueryGraph::Node *child_node = child_nodes[i];
            run_single_node(ctx, child_node);
        }

        break;
    }
    case QueryGraph::Node::Type::VALUE:
        OK_UNREACHABLE();
    }

}

void DBConnection::execute(TypedCompiledQuery *query, QueryResults *results) {
    results->error = {};
    results->value = {};

    ok::Optional<QueryGraph::Node *> cur_node_opt{};

    QueryExecutionContext *ctx = db_pool->rent_empty_execution_context(db, user);

    jmp_buf buf;
    int ret = setjmp(buf);

    memcpy(&ctx->jmpbuf, buf, sizeof(jmp_buf));

    UZ instructions_count = query->untyped.instructions.count;

    if (ret != 0) {
        results->error = ctx->error;
        goto end;
    }

    for (UZ i = 0; i < instructions_count; ++i) {
        execute_instruction(query, i, ctx, this);
    }

    // Run the query graph, NOW!
    cur_node_opt = ctx->query_graph.root_node();
    while (cur_node_opt) {
        QueryGraph::Node *cur_node = cur_node_opt.get();
        run_single_node(ctx, cur_node);
        cur_node_opt = cur_node->next();
    }

    if (ctx->last_emitted_query.has_value()) {
        results->value = ctx->last_emitted_query.value;
    }

end:

    db_pool->return_execution_context(ctx);
}
}; // namespace xmdb
