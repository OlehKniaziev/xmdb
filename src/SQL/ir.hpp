#ifndef XMDB_IR_HPP
#define XMDB_IR_HPP

#include <Core/ok.hpp>
#include <Core/util.hpp>
#include <Core/image.hpp>

#include "ast.hpp"

using ok::Allocator;
using ok::List;
using ok::Optional;
using ok::Slice;
using ok::String;
using ok::StringView;
using ok::Table;

namespace xmdb::SQL {

#define XMDB_ENUM_COLUMN_TYPES \
    X(INTEGER) \
    X(FLOAT) \
    X(DOUBLE) \
    X(TEXT) \
    X(BOOLEAN) \
    X(PNG)

enum class ColumnType {
#define X(type) type,
XMDB_ENUM_COLUMN_TYPES
#undef X
};

const char *column_type_to_string(ColumnType);
ok::Optional<ColumnType> parse_column_type(ok::StringView);

struct TableSchema {
    static TableSchema untyped(Allocator *a) {
        TableSchema schema;
        schema.columns_names = List<Optional<String>>::alloc(a);
        schema.columns_types = {};
        return schema;
    }

    static TableSchema typed(Allocator *a) {
        TableSchema schema = TableSchema::untyped(a);
        schema.columns_types = List<ColumnType>::alloc(a);
        return schema;
    }

    inline bool find_column(StringView name) const {
        for (UZ i = 0; i < columns_names.count; ++i) {
            if (columns_names[i].has_value() && columns_names[i].value == name) return true;
        }

        return false;
    }

    List<Optional<String>> columns_names;
    Optional<List<ColumnType>> columns_types;
};

#define ENUM_IR_CONTRACTS                                                                                              \
    INSTR_VAR_2(Eq, U32, U32)                                                                                          \
    INSTR_VAR_2(Lt, U32, U32)                                                                                          \
    INSTR_VAR_2(Gt, U32, U32)                                                                                          \
    INSTR_VAR_2(FetchColumn, U32, StringView)                                                                          \
    INSTR_VAR_2(FetchTable, StringView, TableSchema *)                                                                 \
    INSTR_VAR_2(Call, StringView, S64) \
    INSTR_3(EmitColumn, U32, U32, StringView)                                                                          \
    INSTR_VAR_1(EmitQuery, U32)                                                                                        \
    INSTR_1(UseDatabase, StringView)                                                                                   \
    INSTR_1(Arg, U32) \
    INSTR_2(CreateTable, StringView, TableSchema *)                                                                    \
    INSTR_1(CreateDatabase, StringView)                                                                                \
    INSTR_1(CreateUser, StringView)                                                                                    \
    INSTR_1(DropTable, StringView)                                                                                     \
    INSTR_1(DropDatabase, StringView)                                                                                  \
    INSTR_3(InsertColumn, U32, U32, StringView)                                                                        \
    INSTR_1(InsertRow, U32)                                                                                            \
    INSTR_0(CommitInsert)                                                                                              \
    INSTR_3(UpdateColumn, U32, U32, StringView)                                                                        \
    INSTR_0(CommitUpdate)                                                                                              \
    INSTR_1(DeleteTable, U32)                                                                                          \
    INSTR_3(AlterUserProperty, StringView, StringView, U32)                                                            \
    INSTR_0(CommitAlterUser)                                                                                           \
    INSTR_VAR_1(ConstInt, S64)                                                                                         \
    INSTR_VAR_1(ConstString, StringView)                                                                               \
    INSTR_VAR_0(ConstTrue)                                                                                             \
    INSTR_VAR_0(ConstFalse)                                                                                            \
    INSTR_VAR_0(ConstNull)

#define INSTR_0(instr) IRInstructionOperator_##instr,
#define INSTR_VAR_0(instr) IRInstructionOperator_##instr,
#define INSTR_1(instr, _op1) IRInstructionOperator_##instr,
#define INSTR_VAR_1(instr, _op1) IRInstructionOperator_##instr,
#define INSTR_2(instr, _op1, _op2) IRInstructionOperator_##instr,
#define INSTR_VAR_2(instr, _op1, _op2) IRInstructionOperator_##instr,
#define INSTR_3(instr, _op1, _op2, _op3) IRInstructionOperator_##instr,
#define INSTR_VAR_3(instr, _op1, _op2, _op3) IRInstructionOperator_##instr,

enum IRInstructionOperator : U32 { ENUM_IR_CONTRACTS };

#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
#undef INSTR_VAR_3
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3

#define INSTR_0(instr)                                                                                                 \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_0(instr)                                                                                             \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_1(instr, _op1)                                                                                           \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_1(instr, _op1)                                                                                       \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_2(instr, _op1, _op2)                                                                                     \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_2(instr, _op1, _op2)                                                                                 \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_3(instr, _op1, _op2, _op3)                                                                               \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_3(instr, _op1, _op2, _op3)                                                                           \
    case IRInstructionOperator_##instr: return #instr;

static inline const char *ir_instruction_operator_name(IRInstructionOperator op) {
    switch (op) { ENUM_IR_CONTRACTS }
    OK_UNREACHABLE();
}

#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
#undef INSTR_VAR_3
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3

#define XMDB_ENUM_BUILTIN_FUNCTIONS \
    X(RGB, ImageChunk, U32, U32, StringView)

struct IRInstruction {
    inline bool is_table_generating() const {
        return op == IRInstructionOperator_FetchTable || op == IRInstructionOperator_EmitQuery;
    }

    IRInstructionOperator op;
    U32 operand1;
    U32 operand2;
    U32 operand3;
};

static_assert(sizeof(IRInstruction) == sizeof(U32) * 4);

struct IREmitter {
    IREmitter() = default;

    explicit IREmitter(Allocator *allocator) : allocator{allocator} {
        instructions = List<IRInstruction>::alloc(allocator);
        tokens = List<Token>::alloc(allocator);
        strings = List<String>::alloc(allocator);
        integers = List<S64>::alloc(allocator);
        schemas = List<TableSchema *>::alloc(allocator);
    }

    inline U32 gen_temp() {
        auto temp_name = String::format(allocator, "temp_%d", temps_count);
        ++temps_count;
        strings.push(temp_name);
        return strings.count - 1;
    }

    inline U32 add_string(String string) {
        strings.push(string);
        return strings.count - 1;
    }

    inline U32 add_string(StringView sv) {
        auto copy = sv.to_string(allocator);
        return add_string(copy);
    }

    inline U32 add_int(S64 integer) {
        integers.push(integer);
        return integers.count - 1;
    }

    inline U32 add_schema(TableSchema *schema) {
        schemas.push(schema);
        return schemas.count - 1;
    }

    inline U32 add_instruction(IRInstruction instr) {
        instructions.push(instr);
        return instructions.count - 1;
    }

    Allocator *allocator;
    U32 temps_count = 0;
    List<IRInstruction> instructions;
    List<Token> tokens;
    List<String> strings;
    List<S64> integers;
    // FIXME: instead of maintaining it's own list, the IREmitter structure should re-use indices
    // into the 'table_schemas' array of some 'DBSchema' object
    List<TableSchema *> schemas;
};

template<typename A, typename B>
struct Tuple {
    A op1;
    B op2;
};

template<typename A, typename B, typename C>
struct Triple {
    A op1;
    B op2;
    C op3;
};

template<typename T>
struct IRContractGetter {};

template<>
struct IRContractGetter<StringView> {
    template<typename Store>
    static StringView get(Store *emitter, U32 op) {
        return emitter->strings[op].view();
    }
};

template<>
struct IRContractGetter<S64> {
    template<typename Store>
    static S64 get(Store *emitter, U32 op) {
        return emitter->integers[op];
    }
};

template<>
struct IRContractGetter<TableSchema *> {
    template<typename Store>
    static TableSchema *get(Store *emitter, U32 op) {
        return emitter->schemas[op];
    }
};

template<>
struct IRContractGetter<U32> {
    template<typename Store>
    static U32 get(Store *emitter, U32 op) {
        OK_UNUSED(emitter);
        return op;
    }
};

#define INSTR_VAR_0(name)                                                                                              \
    template<typename Store>                                                                                           \
    static inline String operands_of_##name(Store *emitter, U32 ip) {                                                  \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        String var_name = emitter->strings[instr.operand1];                                                            \
        return var_name;                                                                                               \
    }

#define INSTR_VAR_1(name, t1)                                                                                          \
    template<typename Store>                                                                                           \
    static inline Tuple<String, t1> operands_of_##name(Store *emitter, U32 ip) {                                       \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        String var_name = emitter->strings[instr.operand1];                                                            \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand2);                                                   \
        return {var_name, op1};                                                                                        \
    }

#define INSTR_VAR_2(name, t1, t2)                                                                                      \
    template<typename Store>                                                                                           \
    static inline Triple<String, t1, t2> operands_of_##name(Store *emitter, U32 ip) {                                  \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        String var_name = emitter->strings[instr.operand1];                                                            \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand2);                                                   \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand3);                                                   \
        return {var_name, op1, op2};                                                                                   \
    }

#define INSTR_0(name)                                                                                                  \
    template<typename Store>                                                                                           \
    static inline void operands_of_##name(Store *emitter, U32 ip) {                                                    \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
    }

#define INSTR_1(name, t1)                                                                                              \
    template<typename Store>                                                                                           \
    static inline t1 operands_of_##name(Store *emitter, U32 ip) {                                                      \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);                                                   \
        return op1;                                                                                                    \
    }

#define INSTR_2(name, t1, t2)                                                                                          \
    template<typename Store>                                                                                           \
    static inline Tuple<t1, t2> operands_of_##name(Store *emitter, U32 ip) {                                           \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);                                                   \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand2);                                                   \
        return {op1, op2};                                                                                             \
    }

#define INSTR_3(name, t1, t2, t3)                                                                                      \
    template<typename Store>                                                                                           \
    static inline Triple<t1, t2, t3> operands_of_##name(Store *emitter, U32 ip) {                                      \
        IRInstruction instr = emitter->instructions[ip];                                                               \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                                                           \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);                                                   \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand2);                                                   \
        t3 op3 = IRContractGetter<t3>::get(emitter, instr.operand3);                                                   \
        return {op1, op2, op3};                                                                                        \
    }

ENUM_IR_CONTRACTS

#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3

struct DBSchema {
    static DBSchema alloc(Allocator *allocator, StringView name) {
        DBSchema schema;
        schema.allocator = allocator;
        schema.name = name.to_string(allocator);
        schema.table_schemas_index = Table<String, U32>::alloc(allocator);
        schema.table_schema_count = 0;
        // This memory will not be commited until we touch it, so allocate *enough*
        // of it upfront, for our convenience.
        schema.table_schemas = (TableSchema *) allocator->raw_alloc(~(U32) 0);
        return schema;
    }

    static DBSchema alloc_default(Allocator *allocator) {
        DBSchema schema;
        schema.allocator = allocator;
        schema.name = String::alloc(allocator, "default");
        schema.table_schemas_index = Table<String, U32>::alloc(allocator);
        schema.table_schema_count = 0;
        // This memory will not be commited until we touch it, so allocate *enough*
        // of it upfront, for our convenience.
        schema.table_schemas = (TableSchema *) allocator->raw_alloc(~(U32) 0);
        return schema;
    }

    inline TableSchema *alloc_table_schema(Optional<String> name, bool typed) {
        if (name) {
            table_schemas_index.put(name.value, table_schema_count);
        }

        TableSchema *schema = &table_schemas[table_schema_count++];
        if (typed) *schema = TableSchema::typed(allocator);
        else
            *schema = TableSchema::untyped(allocator);

        return schema;
    }

    U32 table_schema_count;
    Allocator *allocator;
    // FIXME: don't allocate it as a bigass block of memory, find out a better way
    TableSchema *table_schemas;
    String name;
    Table<String, U32> table_schemas_index;
};

struct IrContext {
    IrContext() = default;

    explicit IrContext(Allocator *allocator, StringView source) :
        allocator{allocator}, source{source}, ir_emitter{allocator} {
        auto default_schema = DBSchema::alloc_default(allocator);
        database_schemas = List<DBSchema>::alloc(allocator);
        database_schemas.push(default_schema);

        table_stack = List<U32>::alloc(allocator);
        namespace_stack = List<Namespace>::alloc(allocator);

        push_namespace(NS_GLOBAL);
    }

    inline DBSchema *active_db_schema() {
        return &database_schemas[active_db_id];
    }

    inline Optional<TableSchema *> get_table_schema(U8 db_schema_id, StringView table_name) {
        auto *db_schema = &database_schemas[db_schema_id];
        Optional<U32> table_schema_id = db_schema->table_schemas_index.get(table_name);
        TRY(table_schema_id);

        OK_ASSERT(table_schema_id.value < db_schema->table_schema_count);

        return &db_schema->table_schemas[table_schema_id.value];
    }

    inline Optional<TableSchema *> get_table_schema_by_id(U32 id) {
        IRInstruction instruction = ir_emitter.instructions[id];

        if (instruction.is_table_generating()) {
            U32 schema_id = instruction.operand3;
            OK_ASSERT(ir_emitter.schemas.count > schema_id);
            return ir_emitter.schemas[schema_id];
        }

        return {};
    }

    inline TableSchema *alloc_table_schema(U8 db_schema_id, Optional<String> table_name, bool typed) {
        DBSchema *db_schema = &database_schemas[db_schema_id];
        TableSchema *table_schema = db_schema->alloc_table_schema(table_name, typed);

        return table_schema;
    }

    enum Namespace : U8 {
        NS_GLOBAL,
        NS_TABLE,
    };

    inline U32 current_table_id() const {
        OK_ASSERT(table_stack.count != 0);
        return table_stack[table_stack.count - 1];
    }

    inline Namespace current_namespace() const {
        OK_ASSERT(namespace_stack.count != 0);
        return namespace_stack[namespace_stack.count - 1];
    }

    inline void push_namespace(Namespace ns) {
        OK_ASSERT(ns != NS_TABLE);
        namespace_stack.push(ns);
    }

    inline void push_table(U32 table_id) {
        table_stack.push(table_id);
        namespace_stack.push(NS_TABLE);
    }

    inline void pop_namespace() {
        auto prev_ns = namespace_stack.pop();
        if (prev_ns == NS_TABLE) table_stack.pop();
    }

    inline void error_on(Token token, String message) {
        ErrorWithSourceLocation error;
        error.location = locate_token(source, token);
        error.message = message;
        this->error = error;
    }

    Allocator *allocator;
    StringView source; // used only for error reporting
    List<DBSchema> database_schemas;
    List<U32> table_stack;
    List<Namespace> namespace_stack;
    IREmitter ir_emitter;
    U32 active_db_id = 0;
    Optional<ErrorWithSourceLocation> error{};
};

struct CompiledQuery {
    Slice<IRInstruction> instructions;
    Slice<Token> tokens;
    Slice<String> strings;
    Slice<S64> integers;
    Slice<TableSchema *> schemas;
};

bool ir_compile_query(Query *in_query, IrContext *ctx, CompiledQuery *out_query);
String stringify_ir(Allocator *, CompiledQuery *);
}; // namespace xmdb::SQL

#endif // XMDB_IR_HPP
