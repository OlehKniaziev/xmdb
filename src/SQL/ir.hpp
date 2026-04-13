#ifndef XMDB_IR_HPP
#define XMDB_IR_HPP

#include <Core/image.hpp>
#include <Core/ok.hpp>
#include <Core/util.hpp>

#include "ast.hpp"

using ok::Allocator;
using ok::List;
using ok::Optional;
using ok::Slice;
using ok::String;
using ok::StringView;
using ok::Table;

namespace xmdb::SQL
{

#define XMDB_ENUM_COLUMN_TYPES                                                 \
    X(INTEGER)                                                                 \
    X(FLOAT)                                                                   \
    X(DOUBLE)                                                                  \
    X(TEXT)                                                                    \
    X(BOOLEAN)                                                                 \
    X(PNG)                                                                     \
    X(MEDIA)

/**
 * @brief Supported types for table columns.
 */
enum class ColumnType
{
#define X(type) type,
    XMDB_ENUM_COLUMN_TYPES
#undef X
};

/**
 * @brief Converts a ColumnType to its string representation.
 * @param type The column type.
 * @return The name of the column type.
 */
const char *column_type_to_string(ColumnType type);

/**
 * @brief Parses a column type from a string view.
 * @param sv The string view to parse.
 * @return The parsed ColumnType, or empty if unknown.
 */
ok::Optional<ColumnType> parse_column_type(ok::StringView sv);

/**
 * @brief Represents the schema of a table in the IR.
 */
struct TableSchema
{
    /**
     * @brief Creates an untyped table schema.
     * @param a The allocator to use.
     * @return The new TableSchema.
     */
    static TableSchema untyped(Allocator *a)
    {
        TableSchema schema;
        schema.columns_names = List<Optional<String>>::alloc(a);
        schema.columns_types = {};
        return schema;
    }

    /**
     * @brief Creates a typed table schema.
     * @param a The allocator to use.
     * @return The new TableSchema.
     */
    static TableSchema typed(Allocator *a)
    {
        TableSchema schema = TableSchema::untyped(a);
        schema.columns_types = List<ColumnType>::alloc(a);
        return schema;
    }

    /**
     * @brief Checks if a column with the given name exists in the schema.
     * @param name The column name to search for.
     * @return true if found, false otherwise.
     */
    inline bool find_column(StringView name) const
    {
        for (UZ i = 0; i < columns_names.count; ++i)
        {
            if (columns_names[i].has_value() && columns_names[i].value == name)
                return true;
        }

        return false;
    }

    List<Optional<String>> columns_names; ///< The names of the columns.
    Optional<List<ColumnType>>
            columns_types; ///< The types of the columns (if typed).
};

#define ENUM_IR_CONTRACTS                                                      \
    INSTR_VAR_2(Eq, U32, U32)                                                  \
    INSTR_VAR_2(Lt, U32, U32)                                                  \
    INSTR_VAR_2(Gt, U32, U32)                                                  \
    INSTR_VAR_2(FetchColumn, U32, StringView)                                  \
    INSTR_VAR_2(FetchTable, StringView, TableSchema *)                         \
    INSTR_VAR_2(Call, StringView, S64)                                         \
    INSTR_3(EmitColumn, U32, U32, StringView)                                  \
    INSTR_VAR_1(EmitQuery, U32)                                                \
    INSTR_1(UseDatabase, StringView)                                           \
    INSTR_1(Arg, U32)                                                          \
    INSTR_2(CreateTable, StringView, TableSchema *)                            \
    INSTR_1(CreateDatabase, StringView)                                        \
    INSTR_1(CreateUser, StringView)                                            \
    INSTR_1(DropTable, StringView)                                             \
    INSTR_1(DropDatabase, StringView)                                          \
    INSTR_2(InsertColumn, StringView, U32)                                     \
    INSTR_0(InsertRow)                                                         \
    INSTR_1(CommitInsert, U32)                                                 \
    INSTR_3(UpdateColumn, U32, U32, StringView)                                \
    INSTR_0(CommitUpdate)                                                      \
    INSTR_1(DeleteTable, U32)                                                  \
    INSTR_3(AlterUserProperty, StringView, StringView, U32)                    \
    INSTR_0(CommitAlterUser)                                                   \
    INSTR_VAR_1(ConstInt, S64)                                                 \
    INSTR_VAR_1(ConstString, StringView)                                       \
    INSTR_VAR_0(ConstTrue)                                                     \
    INSTR_VAR_0(ConstFalse)                                                    \
    INSTR_VAR_0(ConstNull)

#define INSTR_0(instr) IRInstructionOperator_##instr,
#define INSTR_VAR_0(instr) IRInstructionOperator_##instr,
#define INSTR_1(instr, _op1) IRInstructionOperator_##instr,
#define INSTR_VAR_1(instr, _op1) IRInstructionOperator_##instr,
#define INSTR_2(instr, _op1, _op2) IRInstructionOperator_##instr,
#define INSTR_VAR_2(instr, _op1, _op2) IRInstructionOperator_##instr,
#define INSTR_3(instr, _op1, _op2, _op3) IRInstructionOperator_##instr,
#define INSTR_VAR_3(instr, _op1, _op2, _op3) IRInstructionOperator_##instr,

/**
 * @brief Opcodes for IR instructions.
 */
enum IRInstructionOperator : U32
{
    ENUM_IR_CONTRACTS
};

#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
#undef INSTR_VAR_3
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3

#define INSTR_0(instr)                                                         \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_0(instr)                                                     \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_1(instr, _op1)                                                   \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_1(instr, _op1)                                               \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_2(instr, _op1, _op2)                                             \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_2(instr, _op1, _op2)                                         \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_3(instr, _op1, _op2, _op3)                                       \
    case IRInstructionOperator_##instr: return #instr;
#define INSTR_VAR_3(instr, _op1, _op2, _op3)                                   \
    case IRInstructionOperator_##instr: return #instr;

/**
 * @brief Gets the name of an IR instruction operator.
 * @param op The instruction operator.
 * @return The name string.
 */
static inline const char *ir_instruction_operator_name(IRInstructionOperator op)
{
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

#define XMDB_ENUM_BUILTIN_FUNCTIONS                                            \
    X(RGB, ImageChunk, U32, U32, StringView)                                   \
    X(MEDIA, MediaSource, StringView)

#define XMDB_BUILTIN_FUNCTION_SIG_NAME(name, ret, ...)                         \
    Result<ret, ErrorWithSourceLocation> (*name)(SourceLocation,               \
                                                 ok::Allocator *, __VA_ARGS__)
#define XMDB_BUILTIN_FUNCTION_SIG(ret, ...)                                    \
    Result<ret, ErrorWithSourceLocation> (*)(SourceLocation, ok::Allocator *,  \
                                             __VA_ARGS__)

/**
 * @brief Represents a single instruction in the IR.
 */
struct IRInstruction
{
    /**
     * @brief Checks if the instruction generates a new table.
     * @return true if table-generating, false otherwise.
     */
    inline bool is_table_generating() const
    {
        return op == IRInstructionOperator_FetchTable ||
               op == IRInstructionOperator_EmitQuery;
    }

    IRInstructionOperator op; ///< The opcode.
    U32 operand1; ///< First operand.
    U32 operand2; ///< Second operand.
    U32 operand3; ///< Third operand.
};

static_assert(sizeof(IRInstruction) == sizeof(U32) * 4);

/**
 * @brief Emits IR instructions during compilation.
 */
struct IREmitter
{
    IREmitter() = default;

    /**
     * @brief Constructs a new IREmitter.
     * @param allocator The allocator to use.
     */
    explicit IREmitter(Allocator *allocator) : allocator{allocator}
    {
        instructions = List<IRInstruction>::alloc(allocator);
        tokens = List<Token>::alloc(allocator);
        strings = List<String>::alloc(allocator);
        integers = List<S64>::alloc(allocator);
        schemas = List<TableSchema *>::alloc(allocator);
    }

    /**
     * @brief Generates a new unique temporary variable name.
     * @return Index of the name in the strings pool.
     */
    inline U32 gen_temp()
    {
        auto temp_name = String::format(allocator, "temp_%d", temps_count);
        ++temps_count;
        strings.push(temp_name);
        return strings.count - 1;
    }

    /**
     * @brief Adds a string to the strings pool.
     * @param string The string to add.
     * @return The index of the string.
     */
    inline U32 add_string(String string)
    {
        strings.push(string);
        return strings.count - 1;
    }

    /**
     * @brief Adds a string view to the strings pool.
     * @param sv The string view to add.
     * @return The index of the string.
     */
    inline U32 add_string(StringView sv)
    {
        auto copy = sv.to_string(allocator);
        return add_string(copy);
    }

    /**
     * @brief Adds an integer to the integers pool.
     * @param integer The integer to add.
     * @return The index of the integer.
     */
    inline U32 add_int(S64 integer)
    {
        integers.push(integer);
        return integers.count - 1;
    }

    /**
     * @brief Adds a table schema to the schemas pool.
     * @param schema The schema to add.
     * @return The index of the schema.
     */
    inline U32 add_schema(TableSchema *schema)
    {
        schemas.push(schema);
        return schemas.count - 1;
    }

    /**
     * @brief Appends an instruction to the IR.
     * @param instr The instruction to add.
     * @return The index of the instruction.
     */
    inline U32 add_instruction(IRInstruction instr)
    {
        instructions.push(instr);
        return instructions.count - 1;
    }

    Allocator *allocator; ///< The allocator.
    U32 temps_count = 0; ///< Count of temporary variables.
    List<IRInstruction> instructions; ///< The list of instructions.
    List<Token> tokens; ///< Tokens associated with instructions.
    List<String> strings; ///< The string pool.
    List<S64> integers; ///< The integer pool.
    List<TableSchema *> schemas; ///< The schema pool.
};

/**
 * @brief Helper for two values.
 */
template <typename A, typename B>
struct Tuple
{
    A op1;
    B op2;
};

/**
 * @brief Helper for three values.
 */
template <typename A, typename B, typename C>
struct Triple
{
    A op1;
    B op2;
    C op3;
};

/**
 * @brief Template for getting operand values from the emitter store.
 */
template <typename T>
struct IRContractGetter
{
};

template <>
struct IRContractGetter<StringView>
{
    /**
     * @brief Retrieves a string view from the emitter.
     * @param emitter The emitter.
     * @param op The operand index.
     * @return The string view.
     */
    template <typename Store>
    static StringView get(Store *emitter, U32 op)
    {
        return emitter->strings[op].view();
    }
};

template <>
struct IRContractGetter<S64>
{
    /**
     * @brief Retrieves an integer from the emitter.
     * @param emitter The emitter.
     * @param op The operand index.
     * @return The integer value.
     */
    template <typename Store>
    static S64 get(Store *emitter, U32 op)
    {
        return emitter->integers[op];
    }
};

template <>
struct IRContractGetter<TableSchema *>
{
    /**
     * @brief Retrieves a table schema pointer from the emitter.
     * @param emitter The emitter.
     * @param op The operand index.
     * @return The table schema pointer.
     */
    template <typename Store>
    static TableSchema *get(Store *emitter, U32 op)
    {
        return emitter->schemas[op];
    }
};

template <>
struct IRContractGetter<U32>
{
    /**
     * @brief Retrieves a raw U32 operand.
     * @param emitter The emitter (unused).
     * @param op The operand value.
     * @return The operand value.
     */
    template <typename Store>
    static U32 get(Store *emitter, U32 op)
    {
        OK_UNUSED(emitter);
        return op;
    }
};

#define INSTR_VAR_0(name)                                                      \
    template <typename Store>                                                  \
    static inline String operands_of_##name(Store *emitter, U32 ip)            \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        String var_name = emitter->strings[instr.operand1];                    \
        return var_name;                                                       \
    }

#define INSTR_VAR_1(name, t1)                                                  \
    template <typename Store>                                                  \
    static inline Tuple<String, t1> operands_of_##name(Store *emitter, U32 ip) \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        String var_name = emitter->strings[instr.operand1];                    \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand2);           \
        return {var_name, op1};                                                \
    }

#define INSTR_VAR_2(name, t1, t2)                                              \
    template <typename Store>                                                  \
    static inline Triple<String, t1, t2> operands_of_##name(Store *emitter,    \
                                                            U32 ip)            \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        String var_name = emitter->strings[instr.operand1];                    \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand2);           \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand3);           \
        return {var_name, op1, op2};                                           \
    }

#define INSTR_0(name)                                                          \
    template <typename Store>                                                  \
    static inline void operands_of_##name(Store *emitter, U32 ip)              \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
    }

#define INSTR_1(name, t1)                                                      \
    template <typename Store>                                                  \
    static inline t1 operands_of_##name(Store *emitter, U32 ip)                \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);           \
        return op1;                                                            \
    }

#define INSTR_2(name, t1, t2)                                                  \
    template <typename Store>                                                  \
    static inline Tuple<t1, t2> operands_of_##name(Store *emitter, U32 ip)     \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);           \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand2);           \
        return {op1, op2};                                                     \
    }

#define INSTR_3(name, t1, t2, t3)                                              \
    template <typename Store>                                                  \
    static inline Triple<t1, t2, t3> operands_of_##name(Store *emitter,        \
                                                        U32 ip)                \
    {                                                                          \
        IRInstruction instr = emitter->instructions[ip];                       \
        OK_ASSERT(instr.op == IRInstructionOperator_##name);                   \
        t1 op1 = IRContractGetter<t1>::get(emitter, instr.operand1);           \
        t2 op2 = IRContractGetter<t2>::get(emitter, instr.operand2);           \
        t3 op3 = IRContractGetter<t3>::get(emitter, instr.operand3);           \
        return {op1, op2, op3};                                                \
    }

ENUM_IR_CONTRACTS

#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3

/**
 * @brief Represents a database schema in the IR.
 */
struct DBSchema
{
    /**
     * @brief Allocates a new DBSchema.
     * @param allocator The allocator to use.
     * @param name The name of the database.
     * @return The new DBSchema.
     */
    static DBSchema alloc(Allocator *allocator, StringView name)
    {
        DBSchema schema;
        schema.allocator = allocator;
        schema.name = name.to_string(allocator);
        schema.table_schemas_index = Table<String, U32>::alloc(allocator);
        schema.table_schema_count = 0;
        // This memory will not be commited until we touch it, so allocate
        // *enough* of it upfront, for our convenience.
        schema.table_schemas = (TableSchema *) allocator->raw_alloc(~(U32) 0);
        return schema;
    }

    /**
     * @brief Allocates the default DBSchema.
     * @param allocator The allocator to use.
     * @return The default DBSchema.
     */
    static DBSchema alloc_default(Allocator *allocator)
    {
        DBSchema schema;
        schema.allocator = allocator;
        schema.name = String::alloc(allocator, "default");
        schema.table_schemas_index = Table<String, U32>::alloc(allocator);
        schema.table_schema_count = 0;
        // This memory will not be commited until we touch it, so allocate
        // *enough* of it upfront, for our convenience.
        schema.table_schemas = (TableSchema *) allocator->raw_alloc(~(U32) 0);
        return schema;
    }

    /**
     * @brief Allocates a new TableSchema within this DBSchema.
     * @param name The name of the table.
     * @param typed Whether the schema is typed.
     * @return Pointer to the new TableSchema.
     */
    inline TableSchema *alloc_table_schema(Optional<String> name, bool typed)
    {
        if (name)
        {
            table_schemas_index.put(name.value, table_schema_count);
        }

        TableSchema *schema = &table_schemas[table_schema_count++];
        if (typed) *schema = TableSchema::typed(allocator);
        else
            *schema = TableSchema::untyped(allocator);

        return schema;
    }

    U32 table_schema_count; ///< Number of table schemas.
    Allocator *allocator; ///< The allocator.
    TableSchema *table_schemas; ///< Pointer to the array of table schemas.
    String name; ///< Name of the database.
    Table<String, U32> table_schemas_index; ///< Mapping of names to indices.
};

/**
 * @brief Context for compiling AST to IR.
 */
struct IrContext
{
    IrContext() = default;

    /**
     * @brief Constructs a new IrContext.
     * @param allocator The allocator to use.
     * @param source The original SQL source.
     */
    explicit IrContext(Allocator *allocator, StringView source) :
        allocator{allocator}, source{source}, ir_emitter{allocator}
    {
        auto default_schema = DBSchema::alloc_default(allocator);
        database_schemas = List<DBSchema>::alloc(allocator);
        database_schemas.push(default_schema);

        table_stack = List<U32>::alloc(allocator);
        namespace_stack = List<Namespace>::alloc(allocator);

        push_namespace(NS_GLOBAL);
    }

    /**
     * @brief Gets the active database schema.
     * @return Pointer to the active DBSchema.
     */
    inline DBSchema *active_db_schema()
    {
        return &database_schemas[active_db_id];
    }

    /**
     * @brief Retrieves a TableSchema by its name within a database.
     * @param db_schema_id The ID of the database schema.
     * @param table_name The name of the table.
     * @return Optional pointer to the TableSchema.
     */
    inline Optional<TableSchema *> get_table_schema(U8 db_schema_id,
                                                    StringView table_name)
    {
        auto *db_schema = &database_schemas[db_schema_id];
        Optional<U32> table_schema_id =
                db_schema->table_schemas_index.get(table_name);
        TRY(table_schema_id);

        OK_ASSERT(table_schema_id.value < db_schema->table_schema_count);

        return &db_schema->table_schemas[table_schema_id.value];
    }

    /**
     * @brief Retrieves a TableSchema by instruction ID.
     * @param id The instruction ID.
     * @return Optional pointer to the TableSchema.
     */
    inline Optional<TableSchema *> get_table_schema_by_id(U32 id)
    {
        IRInstruction instruction = ir_emitter.instructions[id];

        if (instruction.is_table_generating())
        {
            U32 schema_id = instruction.operand3;
            OK_ASSERT(ir_emitter.schemas.count > schema_id);
            return ir_emitter.schemas[schema_id];
        }

        return {};
    }

    /**
     * @brief Allocates a new TableSchema.
     * @param db_schema_id The database schema ID.
     * @param table_name The name of the table.
     * @param typed Whether the schema is typed.
     * @return Pointer to the new TableSchema.
     */
    inline TableSchema *alloc_table_schema(U8 db_schema_id,
                                           Optional<String> table_name,
                                           bool typed)
    {
        DBSchema *db_schema = &database_schemas[db_schema_id];
        TableSchema *table_schema =
                db_schema->alloc_table_schema(table_name, typed);

        return table_schema;
    }

    /**
     * @brief Namespace types.
     */
    enum Namespace : U8
    {
        NS_GLOBAL,
        NS_TABLE,
    };

    /**
     * @brief Gets the ID of the current table.
     * @return The table ID.
     */
    inline U32 current_table_id() const
    {
        OK_ASSERT(table_stack.count != 0);
        return table_stack[table_stack.count - 1];
    }

    /**
     * @brief Gets the current namespace.
     * @return The namespace.
     */
    inline Namespace current_namespace() const
    {
        OK_ASSERT(namespace_stack.count != 0);
        return namespace_stack[namespace_stack.count - 1];
    }

    /**
     * @brief Pushes a new namespace.
     * @param ns The namespace to push.
     */
    inline void push_namespace(Namespace ns)
    {
        OK_ASSERT(ns != NS_TABLE);
        namespace_stack.push(ns);
    }

    /**
     * @brief Pushes a table namespace.
     * @param table_id The ID of the table.
     */
    inline void push_table(U32 table_id)
    {
        table_stack.push(table_id);
        namespace_stack.push(NS_TABLE);
    }

    /**
     * @brief Pops the current namespace.
     */
    inline void pop_namespace()
    {
        auto prev_ns = namespace_stack.pop();
        if (prev_ns == NS_TABLE) table_stack.pop();
    }

    /**
     * @brief Records an error at a token location.
     * @param token The token where the error occurred.
     * @param message The error message.
     */
    inline void error_on(Token token, String message)
    {
        ErrorWithSourceLocation error;
        error.location = locate_token(source, token);
        error.message = message;
        this->error = error;
    }

    Allocator *allocator; ///< The allocator.
    StringView source; ///< The original source.
    List<DBSchema> database_schemas; ///< List of database schemas.
    List<U32> table_stack; ///< Stack of table IDs.
    List<Namespace> namespace_stack; ///< Stack of namespaces.
    IREmitter ir_emitter; ///< The IR emitter.
    U32 active_db_id = 0; ///< The active database ID.
    Optional<ErrorWithSourceLocation> error{}; ///< Error information.
};

/**
 * @brief Represents a compiled SQL query in IR.
 */
struct CompiledQuery
{
    Slice<IRInstruction> instructions; ///< The instructions.
    Slice<Token> tokens; ///< The tokens.
    Slice<String> strings; ///< The strings.
    Slice<S64> integers; ///< The integers.
    Slice<TableSchema *> schemas; ///< The schemas.
};

/**
 * @brief Compiles a query AST to IR.
 * @param in_query The input query AST.
 * @param ctx The IR context.
 * @param out_query Pointer to store the compiled query.
 * @return true if successful, false otherwise.
 */
bool ir_compile_query(Query *in_query, IrContext *ctx,
                      CompiledQuery *out_query);

/**
 * @brief Stringifies a compiled query.
 * @param allocator The allocator to use for the string.
 * @param query The compiled query.
 * @return The human-readable IR string.
 */
String stringify_ir(Allocator *allocator, CompiledQuery *query);
}; // namespace xmdb::SQL

#endif // XMDB_IR_HPP
