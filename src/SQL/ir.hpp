#ifndef XMDB_IR_HPP
#define XMDB_IR_HPP

#include <Core/ok.hpp>
#include <Core/util.hpp>

#include "ast.hpp"

using ok::Optional;
using ok::String;
using ok::StringView;
using ok::Slice;
using ok::Table;
using ok::Allocator;
using ok::List;

namespace xmdb::SQL {

using U24 = U32;

enum class ColumnType {
    INTEGER,
    FLOAT,
    DOUBLE,
    TEXT,
    IMAGE,
    BOOLEAN,
};

struct TableSchema {
    explicit TableSchema(Allocator* a) {
        column_names = List<Optional<String>>::alloc(a);
        column_types = List<ColumnType>::alloc(a);
    }

    inline bool find_column(StringView name, ColumnType* out_type) const {
        for (UZ i = 0; i < column_names.count; ++i) {
            if (column_names[i].has_value() && column_names[i].value == name) {
                if (out_type != nullptr) *out_type = column_types[i];
                return true;
            }
        }

        return false;
    }

    List<Optional<String>> column_names;
    List<ColumnType> column_types;
};

struct IRInstruction {
    enum Operator : U32 {
        EQ,
        GT,
        LT,

        FETCH_COLUMN,
        FETCH_TABLE,

        EMIT_COLUMN,
        EMIT_QUERY,

        USE_DATABASE,

        CREATE,

        DROP,

        CONST,

        OP_MAX,
    };

    enum Const : U32 {
        CONST_INT,
        CONST_STRING,
        CONST_TRUE,
        CONST_FALSE,
        CONST_NULL,

        CONST_MAX,
    };

    enum Target : U32 {
        TARGET_TABLE,
        TARGET_DATABASE,

        TARGET_MAX,
    };

    inline bool is_table_generating() const {
        return op == FETCH_TABLE || op == EMIT_QUERY;
    }

    Operator op;
    U32 operand1;
    U32 operand2;
    U32 operand3;
};

static_assert(sizeof(IRInstruction) == sizeof(U32) * 4);

struct IREmitter {
    explicit IREmitter(Allocator* allocator) : allocator{allocator} {
        instructions = List<IRInstruction>::alloc(allocator);
        strings = List<String>::alloc(allocator);
        integers = List<S64>::alloc(allocator);
        schemas = List<TableSchema*>::alloc(allocator);
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

    inline U32 add_schema(TableSchema* schema) {
        schemas.push(schema);
        return schemas.count - 1;
    }

    inline U32 add_instruction(IRInstruction instr) {
        instructions.push(instr);
        return instructions.count - 1;
    }

    U32 eq(U32 lhs, U32 rhs) {
        IRInstruction instr;
        instr.op = IRInstruction::EQ;
        instr.operand1 = gen_temp();
        instr.operand2 = lhs;
        instr.operand3 = rhs;
        return add_instruction(instr);
    }

    U32 lt(U32 lhs, U32 rhs) {
        IRInstruction instr;
        instr.op = IRInstruction::LT;
        instr.operand1 = gen_temp();
        instr.operand2 = lhs;
        instr.operand3 = rhs;
        return add_instruction(instr);
    }

    U32 gt(U32 lhs, U32 rhs) {
        IRInstruction instr;
        instr.op = IRInstruction::GT;
        instr.operand1 = gen_temp();
        instr.operand2 = lhs;
        instr.operand3 = rhs;
        return add_instruction(instr);
    }

    void create_database(StringView name) {
        auto db_name = add_string(name);

        IRInstruction instr;
        instr.op = IRInstruction::CREATE;
        instr.operand1 = IRInstruction::TARGET_DATABASE;
        instr.operand2 = db_name;
        instructions.push(instr);
    }

    void create_table(StringView name, TableSchema* schema) {
        auto table_name = add_string(name);
        auto table_schema = add_schema(schema);

        IRInstruction instr;
        instr.op = IRInstruction::CREATE;
        instr.operand1 = IRInstruction::TARGET_TABLE;
        instr.operand2 = table_name;
        instr.operand3 = table_schema;
        instructions.push(instr);
    }

    void drop_database(StringView name) {
        auto db_name = add_string(name);

        IRInstruction instr;
        instr.op = IRInstruction::DROP;
        instr.operand1 = IRInstruction::TARGET_DATABASE;
        instr.operand2 = db_name;
        instructions.push(instr);
    }

    void drop_table(StringView name) {
        auto table_name = add_string(name);

        IRInstruction instr;
        instr.op = IRInstruction::DROP;
        instr.operand1 = IRInstruction::TARGET_TABLE;
        instr.operand2 = table_name;
        instructions.push(instr);
    }

    U32 use_database(StringView name) {
        auto db_name = add_string(name);

        IRInstruction instr;
        instr.op = IRInstruction::USE_DATABASE;
        instr.operand1 = db_name;
        return add_instruction(instr);
    }

    U32 fetch_table(StringView name) {
        auto table_name = add_string(name);

        auto table_var_name_string = String::format(allocator, OK_SV_FMT "_table", OK_SV_ARG(name));
        auto table_var_name = add_string(table_var_name_string);

        IRInstruction instr;
        instr.op = IRInstruction::FETCH_TABLE;
        instr.operand1 = table_var_name;
        instr.operand2 = table_name;
        return add_instruction(instr);
    }

    U32 fetch_column(U32 table, StringView column) {
        auto column_name = add_string(column);

        auto column_var_name_string = String::format(allocator, OK_SV_FMT "_column", OK_SV_ARG(column));
        auto column_var_name = add_string(column_var_name_string);

        IRInstruction instr;
        instr.op = IRInstruction::FETCH_COLUMN;
        instr.operand1 = column_var_name;
        instr.operand2 = table;
        instr.operand3 = column_name;
        return add_instruction(instr);
    }

    void emit_column(U32 column_location, StringView column_name) {
        auto column_name_id = add_string(column_name);

        IRInstruction instr;
        instr.op = IRInstruction::EMIT_COLUMN;
        instr.operand1 = column_location;
        instr.operand2 = column_name_id;

        instructions.push(instr);
    }

    U32 emit_query(U32 columns_count) {
        auto temp_name = gen_temp();

        IRInstruction instr;
        instr.op = IRInstruction::EMIT_QUERY;
        instr.operand1 = temp_name;
        instr.operand2 = columns_count;
        return add_instruction(instr);
    }

    U32 constant(S64 integer) {
        auto temp = gen_temp();
        auto integer_operand = add_int(integer);

        IRInstruction instr;
        instr.op = IRInstruction::CONST;
        instr.operand1 = temp;
        instr.operand2 = IRInstruction::CONST_INT;
        instr.operand3 = integer_operand;
        return add_instruction(instr);
    }

    U32 constant(StringView sv) {
        auto temp = gen_temp();
        auto string_operand = add_string(sv);

        IRInstruction instr;
        instr.op = IRInstruction::CONST;
        instr.operand1 = temp;
        instr.operand2 = IRInstruction::CONST_STRING;
        instr.operand3 = string_operand;
        return add_instruction(instr);
    }

    U32 constant(String string) {
        return constant(string.view());
    }

    U32 constant_true() {
        auto temp = gen_temp();

        IRInstruction instr;
        instr.op = IRInstruction::CONST;
        instr.operand1 = temp;
        instr.operand2 = IRInstruction::CONST_TRUE;
        return add_instruction(instr);
    }

    U32 constant_false() {
        auto temp = gen_temp();

        IRInstruction instr;
        instr.op = IRInstruction::CONST;
        instr.operand1 = temp;
        instr.operand2 = IRInstruction::CONST_FALSE;
        return add_instruction(instr);
    }

    U32 constant_null() {
        auto temp = gen_temp();

        IRInstruction instr;
        instr.op = IRInstruction::CONST;
        instr.operand1 = temp;
        instr.operand2 = IRInstruction::CONST_NULL;
        return add_instruction(instr);
    }

    Allocator* allocator;
    U32 temps_count = 0;
    List<IRInstruction> instructions;
    List<String> strings;
    List<S64> integers;
    // FIXME: instead of maintaining it's own list, the IREmitter structure should re-use indices
    // into the 'table_schemas' array of some 'DBSchema' object
    List<TableSchema*> schemas;
};

struct DBSchema {
    static DBSchema alloc_default(Allocator* allocator) {
        DBSchema schema;
        schema.allocator = allocator;
        schema.name = String::alloc(allocator, "default");
        schema.table_schemas_index = Table<String, U32>::alloc(allocator);
        schema.table_schema_count = 0;
        // This memory will not be commited until we touch it, so allocate *enough*
        // of it upfront, for our convenience.
        schema.table_schemas = (TableSchema*)allocator->raw_alloc(~(U32)0);
        return schema;
    }

    inline TableSchema* alloc_table_schema(Optional<String> name) {
        if (name) {
            OK_ASSERT(!table_schemas_index.has(name.value)); // FIXME
            table_schemas_index.put(name.value, table_schema_count);
        }

        TableSchema* schema = &table_schemas[table_schema_count++];
        *schema = TableSchema{allocator};
        return schema;
    }

    U32 table_schema_count;
    Allocator* allocator;
    TableSchema* table_schemas;
    String name;
    Table<String, U32> table_schemas_index;
};

struct IrContext {
    explicit IrContext(Allocator* allocator) : allocator{allocator}, ir_emitter{allocator} {
        auto default_schema = DBSchema::alloc_default(ok::static_allocator);
        database_schemas = List<DBSchema>::alloc(allocator);
        database_schemas.push(default_schema);

        table_stack = List<U32>::alloc(allocator);
        namespace_stack = List<Namespace>::alloc(allocator);

        push_namespace(NS_GLOBAL);
    }

    inline DBSchema* active_db_schema() {
        return &database_schemas[active_db_id];
    }

    inline Optional<TableSchema*> get_table_schema(U8 db_schema_id, StringView table_name, U24* out_schema_id) {
        auto* db_schema = &database_schemas[db_schema_id];
        Optional<U32> table_schema_id = db_schema->table_schemas_index.get(table_name);
        TRY(table_schema_id);

        OK_ASSERT(table_schema_id.value < db_schema->table_schema_count);

        if (out_schema_id != nullptr)
            *out_schema_id = (U24)db_schema_id << 16 | (U24)table_schema_id.value;

        return &db_schema->table_schemas[table_schema_id.value];
    }

    inline Optional<TableSchema*> get_table_schema_by_id(U32 id) {
        IRInstruction instruction = ir_emitter.instructions[id];

        if (instruction.is_table_generating()) {
            U32 schema_id = instruction.operand3;
            OK_ASSERT(ir_emitter.schemas.count > schema_id);
            return ir_emitter.schemas[schema_id];
        }

        return {};
    }

    inline Optional<TableSchema*> get_table_schema(U8 db_schema_id, U24 table_schema_id) {
        auto* db_schema = &database_schemas[db_schema_id];
        return &db_schema->table_schemas[table_schema_id];
    }

    inline TableSchema* alloc_table_schema(U8 db_schema_id, Optional<String> table_name, U24* out_schema_id) {
        auto* db_schema = &database_schemas[db_schema_id];
        auto* table_schema = db_schema->alloc_table_schema(table_name);
        if (out_schema_id != nullptr)
            *out_schema_id = (U24)db_schema_id << 16 | ((U24)db_schema->table_schema_count - 1);
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

    Allocator* allocator;
    U8 active_db_id = 0;
    List<DBSchema> database_schemas;
    List<U32> table_stack;
    List<Namespace> namespace_stack;
    IREmitter ir_emitter;
};

bool ir_compile_query(Query*, IrContext*);
};

#endif // XMDB_IR_HPP
