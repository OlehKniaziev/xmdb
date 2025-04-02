#include "type_check.hpp"

namespace xmdb::SQL {
static inline bool type_is_comparable(Type type) {
    return type == TYPE_NULL || type == TYPE_INT || type == TYPE_STRING || type == TYPE_BOOL;
}

static inline bool find_column(TypedTableSchema *schema, StringView name, Type *type) {
    for (UZ i = 0; i < schema->column_names.count; ++i) {
        if (schema->column_names[i].has_value() && schema->column_names[i].value == name) {
            *type = schema->column_types[i];
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

static Type const_to_type_table[IRInstruction::CONST_MAX] = {
    [IRInstruction::CONST_INT] = TYPE_INT,
    [IRInstruction::CONST_STRING] = TYPE_STRING,
    [IRInstruction::CONST_TRUE] = TYPE_BOOL,
    [IRInstruction::CONST_FALSE] = TYPE_BOOL,
    [IRInstruction::CONST_NULL] = TYPE_NULL,
};

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

static inline bool type_check_ir_instruction(U32 ip, IREmitter *ir_emitter, TypingContext *ctx) {
    IRInstruction instr = ir_emitter->instructions[ip];

    switch (instr.op) {
    case IRInstruction::LT:
    case IRInstruction::GT:
    case IRInstruction::EQ: {
        Type lhs_type = ctx->ir_instruction_types.get(instr.operand2).get();
        if (!type_is_comparable(lhs_type)) OK_PANIC_FMT("type '%s' is not comparable", type_to_string_table[lhs_type]);
        Type rhs_type = ctx->ir_instruction_types.get(instr.operand3).get();

        if (!types_are_equal(lhs_type, rhs_type))
            OK_PANIC_FMT("cannot compare lhs of type '%s' to rhs of type '%s'", type_to_string_table[lhs_type], type_to_string_table[rhs_type]);

        ctx->ir_instruction_types.put(ip, {TYPE_BOOL});
        return true;
    }
    case IRInstruction::CONST: {
        Type type = const_to_type_table[instr.operand1];
        ctx->ir_instruction_types.put(ip, type);
        return true;
    }
    case IRInstruction::FETCH_TABLE: {
        TableSchema *table_schema = ir_emitter->schemas[instr.operand3];

        OK_ASSERT(table_schema->column_types.has_value());

        List<Type> column_types = List<Type>::alloc(ctx->allocator);

        for (UZ i = 0; i < table_schema->column_types.value.count; ++i) {
            ColumnType column_type = table_schema->column_types.value[i];
            Type type = column_type_to_type_table[column_type];
            column_types.push(type);
        }

        TypedTableSchema typed_table_schema = {
            .column_names = table_schema->column_names.slice(),
            .column_types = column_types.slice(),
        };

        ctx->table_types.put(ip, typed_table_schema);
        return true;
    }
    case IRInstruction::FETCH_COLUMN: {
        U32 table_type_ip = instr.operand2;
        String column_name = ir_emitter->strings[instr.operand3];

        TypedTableSchema table_schema = ctx->table_types.get(table_type_ip).get();

        Type column_type;
        OK_ASSERT(find_column(&table_schema, column_name.view(), &column_type));

        ctx->ir_instruction_types.put(ip, column_type);
        return true;
    }
    case IRInstruction::EMIT_COLUMN: {
        ctx->emitted_columns.push(ip);
        return true;
    }
    case IRInstruction::EMIT_QUERY: {
        U32 columns_to_emit_count = instr.operand2;
        OK_ASSERT(ctx->emitted_columns.count >= columns_to_emit_count);

        List<Optional<String>> column_names = List<Optional<String>>::alloc(ctx->allocator);
        List<Type> column_types = List<Type>::alloc(ctx->allocator);

        for (UZ i = ctx->emitted_columns.count - columns_to_emit_count; i < ctx->emitted_columns.count; ++i) {
            U32 ir_ip = ctx->emitted_columns[i];
            IRInstruction emit_column_instr = ir_emitter->instructions[ir_ip];

            String column_name = ir_emitter->strings[emit_column_instr.operand2];
            Type column_type = ctx->ir_instruction_types.get(emit_column_instr.operand1).get();
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
    case IRInstruction::CREATE:
    case IRInstruction::DROP:
    case IRInstruction::USE_DATABASE:
        return true;
    default: OK_UNREACHABLE();
    }
}

bool type_check_ir(IREmitter *ir_emitter, TypingContext *ctx) {
    for (UZ i = 0; i < ir_emitter->instructions.count; ++i) {
        TRY(type_check_ir_instruction(i, ir_emitter, ctx));
    }

    return true;
}

TypingContext new_typing_context(Allocator *allocator) {
    TypingContext ctx;
    ctx.allocator = allocator;
    ctx.ir_instruction_types = Table<U32, Type>::alloc(allocator);
    ctx.table_types = Table<U32, TypedTableSchema>::alloc(allocator);
    ctx.emitted_columns = List<U32>::alloc(allocator);
    return ctx;
}
}; // namespace xmdb::SQL
