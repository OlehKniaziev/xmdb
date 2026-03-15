#include "type_check.hpp"
#include <SQL/ir.hpp>

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

    if (is_image(lhs) && is_image(rhs)) return true;

    if (lhs != rhs) return false;

    if (lhs == TYPE_TABLE) {
        OK_TODO();
    }

    return true;
}

static void error_on(TypingContext *ctx, Token token, String message) {
    SourceLocation source_location = locate_token(ctx->source, token);
    ctx->error = ErrorWithSourceLocation{.message = message, .location = source_location};
}

Type column_type_to_type(ColumnType column_type) {
    switch (column_type) {
    case ColumnType::INTEGER: return TYPE_INT;
    case ColumnType::FLOAT:   return TYPE_FLOAT;
    case ColumnType::DOUBLE:  return TYPE_DOUBLE;
    case ColumnType::TEXT:    return TYPE_STRING;
    case ColumnType::PNG:     return TYPE_PNG;
    case ColumnType::BOOLEAN: return TYPE_BOOL;
    }

    OK_UNREACHABLE();
}

bool is_image(Type ty) {
    return ty == TYPE_PNG || ty == TYPE_IMAGE_CHUNK;
}

const char *type_name(Type type) {
    switch (type) {
    case TYPE_BOOL:        return "bool";
    case TYPE_STRING:      return "string";
    case TYPE_NULL:        return "null";
    case TYPE_INT:         return "int";
    case TYPE_TABLE:       return "table";
    case TYPE_FLOAT:       return "float";
    case TYPE_DOUBLE:      return "double";
    case TYPE_PNG:         return "PNG";
    case TYPE_IMAGE_CHUNK: return "image";
    }

    OK_UNREACHABLE();
}

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
            String message =
                    String::format(ctx->allocator, "type '%s' is not comparable", type_name(lhs_type));
            error_on(ctx, token, message);
            return false;
        }
        Type rhs_type = ctx->ir_instruction_types.get(operands.op3).get();

        if (!types_are_equal(lhs_type, rhs_type)) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator, "cannot compare lhs of type '%s' to rhs of type '%s'",
                                            type_name(lhs_type), type_name(rhs_type));
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
        Triple<String, StringView, TableSchema *> operands = operands_of_FetchTable(ir_emitter, ip);
        TableSchema *table_schema = operands.op3;

        OK_ASSERT(table_schema->columns_types.has_value());

        List<Type> column_types = List<Type>::alloc(ctx->allocator);

        for (UZ i = 0; i < table_schema->columns_types.value.count; ++i) {
            ColumnType column_type = table_schema->columns_types.value[i];
            Type type = column_type_to_type(column_type);
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
            Triple<U32, U32, StringView> emit_column_ops = operands_of_EmitColumn(ir_emitter, emit_column_ip);

            Type column_type = ctx->ir_instruction_types.get(emit_column_ops.op2).get();

            Optional<String> column_name{};
            if (emit_column_ops.op3.count != 0) column_name = emit_column_ops.op3.to_string(ctx->allocator);

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
            String message =
                    String::format(ctx->allocator, "expected a value of type '%s', but got '%s' instead",
                                   type_name(expected_column_type), type_name(column_type));
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
            String message =
                    String::format(ctx->allocator, "expected a value of type '%s', but got '%s' instead",
                                   type_name(expected_column_type), type_name(column_type));
            error_on(ctx, token, message);
            return false;
        }

        return true;
    }
    case IRInstructionOperator_Arg: {
        U32 value_id = operands_of_Arg(ir_emitter, ip);

        Type arg_type = ctx->ir_instruction_types.get(value_id).get();

        ctx->ir_instruction_types.put(ip, arg_type);

        ctx->call_args.push(ip);

        return true;
    }
    case IRInstructionOperator_Call: {
        Triple<ok::String, ok::StringView, S64> operands = operands_of_Call(ir_emitter, ip);
        StringView fn_name = operands.op2;
        S64 argument_count = operands.op3;

        Optional<FunctionSignature> fn_signature_opt = ctx->function_signatures.get(fn_name);
        if (!fn_signature_opt.has_value()) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator,
                                            "undefined function '" OK_SV_FMT "'",
                                            OK_SV_ARG(fn_name));
            error_on(ctx, token, message);
            return false;
        }

        FunctionSignature signature = fn_signature_opt.get();

        if ((S64) signature.parameter_types.count != argument_count) {
            Token token = ir_emitter->tokens[ip];
            String message = String::format(ctx->allocator,
                                            "function '" OK_SV_FMT "' expects %zu arguments, but %ld provided",
                                            OK_SV_ARG(fn_name),
                                            signature.parameter_types.count,
                                            argument_count);
            error_on(ctx, token, message);
            return false;
        }

        OK_ASSERT((S64) ctx->call_args.count == argument_count);

        for (UZ i = 0; i < ctx->call_args.count; ++i) {
            U32 arg_id = ctx->call_args[i];
            Type arg_type = ctx->ir_instruction_types.get(arg_id).get();
            Type param_type = signature.parameter_types[i];
            if (arg_type != param_type) {
                Token token = ir_emitter->tokens[arg_id];
                String message = String::format(ctx->allocator,
                                                "expected call argument at index %zu to be of type '%s', but got a value of type '%s' instead",
                                                i,
                                                type_name(param_type),
                                                type_name(arg_type));
                error_on(ctx, token, message);
                return false;
            }
        }

        ctx->ir_instruction_types.put(ip, signature.return_type);

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
    case IRInstructionOperator_CreateUser:
    case IRInstructionOperator_AlterUserProperty:
    case IRInstructionOperator_CommitAlterUser:   return true;
    }

    OK_UNREACHABLE();
}

bool type_check_query(CompiledQuery *query, TypingContext *ctx, TypedCompiledQuery *typed_query) {
    for (UZ i = 0; i < query->instructions.count; ++i) {
        TRY(type_check_ir_instruction(i, query, ctx));
    }

    typed_query->untyped = *query;
    typed_query->table_types = ctx->table_types;

    return true;
}

template <typename T>
Type cpp_to_tt() = delete;

template <>
Type cpp_to_tt<ok::StringView>() {
    return TYPE_STRING;
}

template <>
Type cpp_to_tt<U32>() {
    return TYPE_INT;
}

template <>
Type cpp_to_tt<ImageChunk>() {
    return TYPE_IMAGE_CHUNK;
}

template <typename... Args>
Slice<Type> cpp_to_tt_multiple(ok::Allocator *allocator) {
    ok::List<Type> types = ok::List<Type>::alloc(allocator);
    (..., types.push(cpp_to_tt<Args>()));
    return types.slice();
}

TypingContext::TypingContext(Allocator *allocator, StringView source) : allocator{allocator}, source{source} {
    ir_instruction_types = Table<U32, Type>::alloc(allocator);
    table_types = Table<U32, TypedTableSchema>::alloc(allocator);
    function_signatures = Table<StringView, FunctionSignature>::alloc(allocator);
    emitted_columns = List<U32>::alloc(allocator);
    call_args = List<U32>::alloc(allocator);

#define X(fn_name, ret_type, ...) do { \
    Type return_type = cpp_to_tt<ret_type>(); \
    Slice<Type> parameter_types = cpp_to_tt_multiple<__VA_ARGS__>(allocator); \
    function_signatures.put(ok::StringView{#fn_name}, FunctionSignature{return_type, parameter_types}); \
    } while (false);
XMDB_ENUM_BUILTIN_FUNCTIONS
#undef X
}
}; // namespace xmdb::SQL
