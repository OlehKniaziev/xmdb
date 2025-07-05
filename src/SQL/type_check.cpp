#include "type_check.hpp"

namespace xmdb::SQL {
static inline bool type_is_comparable(Type type) {
    return type == TYPE_NULL || type == TYPE_INT || type == TYPE_STRING || type == TYPE_BOOL;
}

static inline bool find_column(TypedTableSchema schema, StringView name, Type *type) {
    for (UZ i = 0; i < schema.column_names.count; ++i) {
        if (schema.column_names[i].has_value() && schema.column_names[i].value == name) {
            *type = schema.column_types[i];
            return true;
        }
    }

    return false;
}

static inline bool types_are_equal(Type lhs, Type rhs) {
    if (lhs == TYPE_NULL || rhs == TYPE_NULL) return true;

    if (lhs != rhs) return false;

    if (lhs == TYPE_TABLE) {
        OK_TODO();
    }

    return true;
}

static void error_on(TypingContext *ctx, Token token, String message) {
    SourceLocation source_location = locate_token(ctx->source, token);
    ctx->error = TypingContextError { .location = source_location, .message = message };
}

static Type column_type_to_type_table[COLUMN_MAX] = {
    [COLUMN_INTEGER] = TYPE_INT,
    [COLUMN_FLOAT] = TYPE_FLOAT,
    [COLUMN_DOUBLE] = TYPE_DOUBLE,
    [COLUMN_TEXT] = TYPE_STRING,
    [COLUMN_IMAGE] = TYPE_IMAGE,
    [COLUMN_BOOLEAN] = TYPE_BOOL,
};

static const char *type_to_string_table[TYPE_MAX] = {
    [TYPE_INT] = "integer",
    [TYPE_STRING] = "string",
    [TYPE_IMAGE] = "image",
    [TYPE_BOOL] = "bool",
    [TYPE_NULL] = "NULL",
    [TYPE_TABLE] = "table",
};

static inline bool type_check_ir_instruction(U32 ip, CompiledQuery *ir_emitter, TypingContext *ctx) {
    IRInstruction instr = ir_emitter->instructions[ip];

    switch (instr.op) {
    case IRInstructionOperator_Lt:
    case IRInstructionOperator_Gt:
    case IRInstructionOperator_Eq: {
        Triple<String, U32, U32> operands = operands_of_Eq(ir_emitter, ip);

        Type lhs_type = ctx->ir_instruction_types.get(operands.op2).get();
        if (!type_is_comparable(lhs_type)) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator, "type '%s' is not comparable", type_to_string_table[lhs_type]);
            error_on(ctx, token, message);
            return false;
        }
        Type rhs_type = ctx->ir_instruction_types.get(operands.op3).get();

        if (!types_are_equal(lhs_type, rhs_type)) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator, "cannot compare lhs of type '%s' to rhs of type '%s'", type_to_string_table[lhs_type], type_to_string_table[rhs_type]);
            error_on(ctx, token, message);
            return false;
        }

        ctx->ir_instruction_types.put(ip, {TYPE_BOOL});
        return true;
    }
    case IRInstructionOperator_ConstInt: {
        ctx->ir_instruction_types.put(ip, TYPE_INT);
        return true;
    }
    case IRInstructionOperator_ConstString: {
        ctx->ir_instruction_types.put(ip, TYPE_STRING);
        return true;
    }
    case IRInstructionOperator_ConstTrue: {
        ctx->ir_instruction_types.put(ip, TYPE_BOOL);
        return true;
    }
    case IRInstructionOperator_ConstFalse: {
        ctx->ir_instruction_types.put(ip, TYPE_BOOL);
        return true;
    }
    case IRInstructionOperator_ConstNull: {
        ctx->ir_instruction_types.put(ip, TYPE_NULL);
        return true;
    }
    case IRInstructionOperator_FetchTable: {
        Triple<String, StringView, TableSchema*> operands = operands_of_FetchTable(ir_emitter, ip);
        TableSchema *table_schema = operands.op3;

        OK_ASSERT(table_schema->columns_types.has_value());

        List<Type> column_types = List<Type>::alloc(ctx->allocator);

        for (UZ i = 0; i < table_schema->columns_types.value.count; ++i) {
            ColumnType column_type = table_schema->columns_types.value[i];
            Type type = column_type_to_type_table[column_type];
            column_types.push(type);
        }

        TypedTableSchema typed_table_schema = {
            .column_names = table_schema->columns_names.slice(),
            .column_types = column_types.slice(),
        };

        ctx->table_types.put(ip, typed_table_schema);
        return true;
    }
    case IRInstructionOperator_FetchColumn: {
        Triple<String, U32, StringView> operands = operands_of_FetchColumn(ir_emitter, ip);

        U32 table_type_ip = operands.op2;
        StringView column_name = operands.op3;

        TypedTableSchema table_schema = ctx->table_types.get(table_type_ip).get();

        Type column_type;
        OK_ASSERT(find_column(table_schema, column_name, &column_type));

        ctx->ir_instruction_types.put(ip, column_type);
        return true;
    }
    case IRInstructionOperator_EmitColumn: {
        ctx->emitted_columns.push(ip);
        return true;
    }
    case IRInstructionOperator_EmitQuery: {
        Tuple<String, U32> operands = operands_of_EmitQuery(ir_emitter, ip);
        U32 columns_to_emit_count = operands.op2;
        OK_ASSERT(ctx->emitted_columns.count >= columns_to_emit_count);

        List<Optional<String>> column_names = List<Optional<String>>::alloc(ctx->allocator);
        List<Type> column_types = List<Type>::alloc(ctx->allocator);

        for (UZ i = ctx->emitted_columns.count - columns_to_emit_count; i < ctx->emitted_columns.count; ++i) {
            U32 emit_column_ip = ctx->emitted_columns[i];
            Tuple<U32, StringView> emit_column_ops = operands_of_EmitColumn(ir_emitter, emit_column_ip);

            Type column_type = ctx->ir_instruction_types.get(emit_column_ops.op1).get();

            Optional<String> column_name{};
            if (emit_column_ops.op2.count != 0)
                column_name = emit_column_ops.op2.to_string(ctx->allocator);

            column_names.push(column_name);
            column_types.push(column_type);
        }

        TypedTableSchema typed_schema = {
            .column_names = column_names.slice(),
            .column_types = column_types.slice(),
        };

        ctx->emitted_columns.count -= columns_to_emit_count;

        ctx->table_types.put(ip, typed_schema);
        return true;
    }
    case IRInstructionOperator_InsertColumn: {
        Triple<U32, U32, StringView> operands = operands_of_InsertColumn(ir_emitter, ip);

        TypedTableSchema table_schema = ctx->table_types.get(operands.op1).get();

        Type column_type = ctx->ir_instruction_types.get(operands.op2).get();

        Type expected_column_type;
        OK_ASSERT(find_column(table_schema, operands.op3, &expected_column_type));

        if (!types_are_equal(column_type, expected_column_type)) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator,
                                            "expected a value of type '%s', but got '%s' instead",
                                            type_to_string_table[expected_column_type],
                                            type_to_string_table[column_type]);
            error_on(ctx, token, message);
            return false;
        }

        return true;
    }
    case IRInstructionOperator_UpdateColumn: {
        Triple<U32, U32, StringView> operands = operands_of_UpdateColumn(ir_emitter, ip);

        TypedTableSchema table_schema = ctx->table_types.get(operands.op1).get();

        Type column_type = ctx->ir_instruction_types.get(operands.op2).get();

        Type expected_column_type;
        OK_ASSERT(find_column(table_schema, operands.op3, &expected_column_type));

        if (!types_are_equal(column_type, expected_column_type)) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator,
                                            "expected a value of type '%s', but got '%s' instead",
                                            type_to_string_table[expected_column_type],
                                            type_to_string_table[column_type]);
            error_on(ctx, token, message);
            return false;
        }

        return true;
    }
    case IRInstructionOperator_CreateTable:
    case IRInstructionOperator_CreateDatabase:
    case IRInstructionOperator_DropTable:
    case IRInstructionOperator_DropDatabase:
    case IRInstructionOperator_UseDatabase:
    case IRInstructionOperator_InsertRow:
    case IRInstructionOperator_CommitInsert:
    case IRInstructionOperator_CommitUpdate:
    case IRInstructionOperator_DeleteTable:
        return true;
    default: OK_UNREACHABLE();
    }
}

bool type_check_query(CompiledQuery *query, TypingContext *ctx, TypedCompiledQuery *typed_query) {
    for (UZ i = 0; i < query->instructions.count; ++i) {
        TRY(type_check_ir_instruction(i, query, ctx));
    }

    typed_query->untyped = *query;
    typed_query->table_types = ctx->table_types;

    return true;
}

TypingContext::TypingContext(Allocator *allocator, StringView source) : allocator{allocator}, source{source} {
    ir_instruction_types = Table<U32, Type>::alloc(allocator);
    table_types = Table<U32, TypedTableSchema>::alloc(allocator);
    emitted_columns = List<U32>::alloc(allocator);
}

}; // namespace xmdb::SQL
