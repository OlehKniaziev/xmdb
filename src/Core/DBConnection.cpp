#include "DBConnection.hpp"
#include "Logger.hpp"
#include "DBRecord.hpp"
#include "constants.hpp"
#include "Result.hpp"
#include "image.hpp"
#include "Png.hpp"
#include "DBRecord.hpp"

#include <SQL/ir.hpp>
#include <SQL/util.hpp>
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
    case TYPE_PNG:    return ColumnType::PNG;
    case TYPE_BOOL:   return ColumnType::BOOLEAN;
    case TYPE_FLOAT:  return ColumnType::FLOAT;
    case TYPE_DOUBLE: return ColumnType::DOUBLE;

    case TYPE_IMAGE_CHUNK:
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
        Tuple<StringView, U32> operands = operands_of_InsertColumn(&query->untyped, i);

        DBValue *column_value = ctx->fetch_var(operands.op2);
        StringView column_name = operands.op1;

        ctx->insert_column(column_name, column_value);
        break;
    }
    case IRInstructionOperator_InsertRow: {
        ctx->insert_row();
        break;
    }
    case IRInstructionOperator_CommitInsert: { // NOTE(oleh): This instruction has no operands!
        U32 table_ip = operands_of_CommitInsert(&query->untyped, i);
        DBTable *table = ctx->fetch_table(table_ip);
        ctx->commit_insert(table);
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

        ctx->sync_state();

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

        ctx->sync_state();

        break;
    }
    case IRInstructionOperator_Arg: {
        U32 value_id = operands_of_Arg(&query->untyped, i);

        DBValue *value = ctx->fetch_var(value_id);
        ctx->prepare_call_arg(value);

        break;
    }
    case IRInstructionOperator_Call: {
        Triple<String, StringView, S64> operands = operands_of_Call(&query->untyped, i);
        ok::StringView var_name = operands.op1.view();

        DBValue *return_value = ctx->call(operands.op2, (U64) operands.op3);
        ctx->put_var(i, var_name, return_value);

        break;
    }
    }
}

template <typename T>
DBValue *cpp_to_db_value(ok::Allocator *, const T&) = delete;

template <>
DBValue *cpp_to_db_value<ImageChunk>(ok::Allocator *allocator, const ImageChunk& image_chunk) {
    return new (allocator) ImageDataDBValue{
        image_chunk.width,
        image_chunk.height,
        image_chunk.data,
        image_chunk.pixel_format,
    };
}

template <typename... Args>
consteval UZ args_count() {
    return sizeof...(Args);
}

template <typename T>
T extract(Allocator *allocator, DBValue *) = delete;

template <UZ Idx, typename T>
T extract(Allocator *allocator, Slice<DBValue *> values) {
    return extract<T>(allocator, values[Idx]);
}

template <>
U32 extract<U32>(Allocator *allocator, DBValue *db_value) {
    Value value = poll(allocator, db_value).get();
    OK_ASSERT(value.type() == Value::Type::INT);

    S64 i = value.as_int();

    OK_VERIFY(i >= 0);
    OK_VERIFY(i < UINT32_MAX);

    return static_cast<U32>(i);
}

template <>
StringView extract<StringView>(Allocator *allocator, DBValue *db_value) {
    Value value = poll(allocator, db_value).get();

    if (value.type() == Value::Type::STRING) {
        const FixedString *s = value.as_string_ptr();
        return s->view();
    } else if (value.type() == Value::Type::BIG_STRING) {
        return value.as_big_string()->view();
    } else {
        OK_PANIC("value was expected to be of a string type");
    }
}

template <UZ... Xs>
struct Seq {};

template <UZ Counter, UZ Max, UZ... Ints>
struct SeqLoop {
    using Type = typename SeqLoop<Counter + 1, Max, Ints..., Counter>::Type;
};

template <UZ Max, UZ... Ints>
struct SeqLoop<Max, Max, Ints...> {
    using Type = Seq<Ints...>;
};

template <typename... args>
using MakeSeq = typename SeqLoop<0, sizeof...(args)>::Type;

template <typename Return, typename... Args, UZ... I>
Result<Return, ErrorWithSourceLocation> call_impl(SourceLocation location,
                                                  ok::Allocator *allocator,
                                                  XMDB_BUILTIN_FUNCTION_SIG_NAME(fn_ptr, Return, Args...),
                                                  ok::Array<DBValue *, sizeof...(Args)> raw_args,
                                                  Seq<I...>) {
    Slice<DBValue *> raw_args_slice = raw_args.slice();
    return fn_ptr(location, allocator, extract<I, Args>(allocator, raw_args_slice)...);
}

template <typename Return, typename... Args>
Result<Return, ErrorWithSourceLocation> call(SourceLocation location,
                                             ok::Allocator *allocator,
                                             XMDB_BUILTIN_FUNCTION_SIG_NAME(fn_ptr, Return, Args...),
                                             ok::Array<DBValue *, sizeof...(Args)> raw_args) {
    return call_impl(location, allocator, fn_ptr, raw_args, MakeSeq<Args...>{});
}

static void fill_column(DBRecord *record,
                        DBTable *table,
                        UZ column_index,
                        Value column_value) {
    TableLayout table_layout = table->layout();

    OK_VERIFY(column_index < table_layout.columns.count);

    ColumnLayout column_layout = table_layout.columns[column_index];
    OK_ASSERT(column_layout.index == column_index);

    OK_VERIFY(column_layout.offset + column_layout.size <= record->buffer_size);

    switch (column_value.type()) {
    case Value::Type::INT: {
        S64 i = column_value.as_int();
        OK_VERIFY(sizeof(i) == column_layout.size);

        UZ o = column_layout.offset;

        for (SZ bit_offset = 56; bit_offset >= 0; bit_offset -= 8) {
            U8 b = ((U64) i >> bit_offset) & 0xFF;
            record->buffer[o] = b;
            ++o;
        }

        if (column_index == table_layout.primary_key_index) {
            record->primary_key_value = (U64)i;
        }

        break;
    }
    case Value::Type::BOOL: {
        bool b = column_value.as_bool();
        OK_VERIFY(sizeof(b) == 1);
        OK_VERIFY(sizeof(b) == column_layout.size);

        record->buffer[column_layout.offset] = b;
        break;
    }
    case Value::Type::STRING: {
        FixedString fs = column_value.as_string();

        OK_VERIFY(sizeof(fs) == column_layout.size);

        memcpy(&record->buffer[column_layout.offset], reinterpret_cast<U8 *>(&fs), sizeof(fs));

        break;
    }
    case Value::Type::IMAGE_CHUNK: {
        ImageChunk *input_chunk = column_value.as_chunk();

        if (input_chunk->x != 0) OK_TODO_MSG("Non-zero chunk x");
        if (input_chunk->y != 0) OK_TODO_MSG("Non-zero chunk y");

        if (input_chunk->data.count > MAX_DISK_IMAGE_CHUNK_DATA_SIZE) OK_TODO_MSG("Image too big");

        ok::Slice<ColumnAttribute> attrs = table->column_attributes();
        ColumnAttribute *attr = &attrs[column_index];

        OK_ASSERT(attr->flags & ColumnAttribute::F_IMAGE);

        ImageColumnState *image_state = &attr->u.image_state;

        U64 new_chunk_index = image_state->header.chunks_count;

        DiskImageChunk chunk = {
            .x = input_chunk->x,
            .y = input_chunk->y,
            .width = input_chunk->width,
            .height = input_chunk->height,
        };

        image_state->file.seek_to(IMAGE_COLUMN_STATE_HEADER_SIZE + MAX_DISK_IMAGE_CHUNK_SIZE * new_chunk_index);

        UZ n_written = 0;
        // @Perf I'm not sure if it's better to do 2 writes of header and data or memcpy potentially 64MiB pixel data.
        // Need to benchmark this or choose the strategy dynamically.
        ok::Optional<ok::File::WriteError> err = image_state->file.write(reinterpret_cast<U8 *>(&chunk), sizeof(chunk), &n_written);
        OK_VERIFY(!err);
        OK_VERIFY(n_written == sizeof(chunk));

        // NOTE(oleh): Need to seek manually here, since the 'write' method seeks back to the original offset.
        image_state->file.seek_to(image_state->file.offset + sizeof(chunk));

        err = image_state->file.write(input_chunk->data.items, input_chunk->data.count, &n_written);
        OK_VERIFY(!err);
        OK_VERIFY(n_written == input_chunk->data.count);

        if (input_chunk->data.count < MAX_DISK_IMAGE_CHUNK_DATA_SIZE) {
            ok::Optional<ok::File::IOError> err =
                image_state->file.truncate(IMAGE_COLUMN_STATE_HEADER_SIZE + MAX_DISK_IMAGE_CHUNK_SIZE * (new_chunk_index + 1));
            OK_VERIFY(!err);
        }

        ++image_state->header.chunks_count;

        OK_VERIFY(image_state_sync(image_state));

        OK_VERIFY(table->columns_types()[column_index] == SQL::ColumnType::PNG);

        Png png = {
            .header = {
                .width = input_chunk->width,
                .height = input_chunk->height,
            },
            .format = input_chunk->pixel_format,
            .indices = {
                .count = 1,
                .items = {(U32) new_chunk_index},
            },
        };

        OK_VERIFY(sizeof(png) == column_layout.size);

        memcpy(&record->buffer[column_layout.offset], &png, sizeof(png));

        break;
    }
    case Value::Type::BIG_STRING: {
        OK_UNREACHABLE();
    }
    }
}

// TODO(oleh): This should depend on the size of a single record for a given table, since we don't
// want to exceed some given memory usage quota.
constexpr UZ DB_RECORD_BUFFER_CAPACITY = 32;

static void flush_buffer(QueryExecutionContext *ctx,
                         DBTable *table,
                         ok::Slice<DBRecord> record_buffer) {
    OK_UNUSED(ctx);

    OK_ASSERT(table->flags() & DBTable::F_PERSIST);

    TableLayout layout = table->layout();
    BTreeIndex *table_index = table->index();
    DBState *state = table->state();

    UZ record_size = table_record_size(layout);

    UZ n_written = 0;

    // TODO(oleh): Instead of doing a write for each record, coalesce them as much as possible.
    for (UZ i = 0; i < record_buffer.count; ++i) {
        n_written = 0;

        DBRecord record = record_buffer[i];
        U64 key = record.primary_key_value;
        table_index->insert(key);

        state->file.seek_to(DB_STATE_HEADER_LENGTH + record_size * (i + state->header.record_count));

        ok::Optional<ok::File::WriteError> write_err = state->file.write(reinterpret_cast<U8 *>(record.buffer), record_size, &n_written);
        OK_VERIFY(!write_err);
        OK_VERIFY(record_size == n_written);
    }

    state->header.record_count += record_buffer.count;
    OK_VERIFY(db_state_sync(state));
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
            DBValue *password_db_value = alter_node->property_value();
            DBTableStream password_value_stream = DBTableStream::from_value(ctx->allocator, password_db_value);

            Value password_value = password_value_stream.next().get();
            FixedString new_password = password_value.as_string();
            user->sha256_password_digest = sha256_digest(new_password.view());

            break;
        }
        }

        ctx->sync_state();

        break;
    }
    case QueryGraph::Node::Type::INSERT: {
        auto *insert_node = static_cast<QueryGraph::InsertNode *>(node);

        DBTable *table = insert_node->table();
        Slice<StringView> insert_column_names = insert_node->column_names();
        Slice<DBValue *> insert_column_values = insert_node->column_values();
        UZ rows_count = insert_node->rows_count();

        if (table->flags() & DBTable::F_PERSIST) {
            TableLayout table_layout = table->layout();

            DBRecord *records_items = ctx->allocator->alloc<DBRecord>(DB_RECORD_BUFFER_CAPACITY);
            Slice<DBRecord> records_buffer = {records_items, DB_RECORD_BUFFER_CAPACITY};
            UZ records_buffer_count = 0;

            for (UZ row_index = 0; row_index < rows_count; ++row_index) {
                DBRecord record = db_record_create(ctx->allocator, table_layout);

                for (UZ column_index = 0; column_index < insert_column_names.count; ++column_index) {
                    DBValue *column_db_value = insert_column_values[row_index * insert_column_names.count + column_index];
                    ok::Optional<Value> column_value = poll(ctx->allocator, column_db_value);
                    OK_VERIFY(column_value);
                    fill_column(&record, table, column_index, column_value.get());
                }

                if (records_buffer_count >= records_buffer.count) {
                    flush_buffer(ctx, table, records_buffer);
                    records_buffer_count = 0;
                } else {
                    records_buffer[records_buffer_count] = record;
                    ++records_buffer_count;
                }
            }

            flush_buffer(ctx, table, records_buffer.slice(0, records_buffer_count));
        } else {
            OK_TODO();
#if OLD_CODE
            Slice<DBValue *> table_columns_values = table->proxy_column_values();

            for (UZ row_idx = 0; row_idx < rows.count; ++row_idx) {
                UZ columns_to_insert_count = rows_to_insert_columns_count[row_idx];
                StringView *columns_to_insert_names = rows_to_insert_columns_names[row_idx];
                DBValue **columns_to_insert_values = rows_to_insert_columns_values[row_idx];

                for (UZ column_to_insert_idx = 0;
                     column_to_insert_idx < columns_to_insert_count;
                     ++column_to_insert_idx) {
                    StringView column_name = columns_to_insert_names[column_to_insert_idx];
                    for (UZ column_idx = 0; column_idx < table->columns_count(); ++column_idx) {
                        if (table->columns_names()[column_idx] != column_name) continue;

                        DBValue *old_value = table_columns_values[column_idx];
                        DBValue *value_to_insert = columns_to_insert_values[column_idx];
                        DBValue *new_value = new (allocator) ConcatDBValue{old_value, value_to_insert};

                        table_columns_values[column_idx] = new_value;
                        break;
                    }
                }
            }

            table->set_rows_count(table->rows_count() + rows.count);
#endif // 0
        }

        break;
    }
    case QueryGraph::Node::Type::CALL: {
        auto *call_node = static_cast<QueryGraph::CallNode *>(node);
        StringView fn_name = call_node->fn_name();
        Slice<DBValue *> call_args = call_node->args();

#define X(name, ret, ...) do {                                          \
            if (fn_name == StringView{#name}) {                         \
                void *raw_ptr = ctx->static_storage->builtin_functions.get(StringView{#name}).get(); \
                auto fn_ptr = reinterpret_cast<XMDB_BUILTIN_FUNCTION_SIG(ret, __VA_ARGS__)>(raw_ptr); \
                ok::Array<DBValue *, args_count<__VA_ARGS__>()> args_array{}; \
                OK_VERIFY(call_args.count == args_array.get_count());   \
                for (UZ i = 0; i < call_args.count; ++i) {              \
                    args_array.items[i] = call_args[i];                 \
                }                                                       \
                XMDB_FIXME("Propagate source location to built-in functions"); \
                Result<ret, ErrorWithSourceLocation> call_result = call<ret, __VA_ARGS__>({}, ctx->allocator, fn_ptr, args_array); \
                if (!call_result.ok()) {                                \
                    ErrorWithSourceLocation err = call_result.error();  \
                    ok::String error_message = SQL::format_error(ok::temp_allocator(), err.location, err.message.view()); \
                    XMDB_FAIL_FMT(ctx, "Failed to call builtin function '%s': %s", \
                                  #name,                             \
                                  error_message.cstr());                \
                } else {                                                \
                    ret builtin_result = call_result.unwrap();          \
                    DBValue *builtin_value_result = cpp_to_db_value(ctx->allocator, builtin_result); \
                    DelayedDBValue *call_node_return_value = call_node->return_value(); \
                    call_node_return_value->set(builtin_value_result);  \
                }                                                       \
                goto success;                                           \
            }                                                           \
        } while (false);

        XMDB_ENUM_BUILTIN_FUNCTIONS

#undef X

        OK_TODO_MSG("User-defined function");

success:
        break;
    }
    case QueryGraph::Node::Type::EMIT_QUERY: {
        auto *emit_node = static_cast<QueryGraph::EmitQueryNode *>(node);

        Slice<DBTable *> queried_tables = emit_node->queried_tables();
        Slice<DBValue *> column_values = emit_node->column_values();
        DBTable *emitted_table = emit_node->table();

        // NOTE(oleh): This is gonna be wrong once the row filtering is added.
        UZ rows_count = 0;

        for (UZ i = 0; i < queried_tables.count; ++i) {
            DBTable *table = queried_tables[i];
            if (table->rows_count() > rows_count) {
                rows_count = table->rows_count();
            }
        }

        emitted_table->set_proxy_column_values(column_values);
        emitted_table->set_proxy_rows_count(rows_count);

        break;
    }
    case QueryGraph::Node::Type::DELETE_TABLE: {
        auto *delete_node = static_cast<QueryGraph::DeleteTableNode *>(node);

        DBTable *table = delete_node->table();

        if (table->flags() & DBTable::F_PROXY) {
            ok::Slice<DBValue *> values = table->proxy_column_values();

            for (UZ i = 0; i < table->columns_count(); ++i) {
                DBValue *new_value = new (ctx->allocator) NoneDBValue{};
                values[i] = new_value;
            }

            table->set_proxy_rows_count(0);
        } else {
            BTreeIndex *index = table->index();
            OK_VERIFY(index->reset());

            DBState *state = table->state();
            OK_VERIFY(db_state_reset(state));
        }

        break;
    }
    case QueryGraph::Node::Type::READ:
        OK_TODO_MSG("READ");
    case QueryGraph::Node::Type::WRITE:
        OK_TODO_MSG("WRITE");
    case QueryGraph::Node::Type::ATOMIC: {
        XMDB_FIXME("Atomic nodes are not atomic at all currently");

        QueryGraph::AtomicNode *atomic_node = static_cast<QueryGraph::AtomicNode *>(node);

        ok::Slice<QueryGraph::Node *> child_nodes = atomic_node->nodes();

        for (UZ i = 0; i < child_nodes.count; ++i) {
            QueryGraph::Node *child_node = child_nodes[i];
            run_single_node(ctx, child_node);
        }

        break;
    }
    case QueryGraph::Node::Type::WRITE_COLUMN: {
        auto *wc_node = static_cast<QueryGraph::WriteColumnNode *>(node);
        DBTable *table = wc_node->table();
        UZ col_idx = wc_node->idx();
        DBValue *col_value = wc_node->value();
        DBState *state = table->state();

        TableLayout layout = table->layout();
        ColumnLayout col = layout.columns[col_idx];
        UZ record_size = table_record_size(layout);

        DBTableStream col_stream = DBTableStream::from_value(ctx->allocator, col_value);

        UZ rows_count = table->rows_count();

        DBRecord record = db_record_create(ctx->allocator, layout);

        for (UZ row_idx = 0; row_idx < rows_count; ++row_idx) {
            ok::Optional<Value> val_opt = col_stream.next();
            OK_VERIFY(val_opt);

            Value val = val_opt.get();
            fill_column(&record, table, col_idx, val);
            state->file.seek_to(DB_STATE_HEADER_LENGTH + record_size * row_idx + col.offset);

            UZ n_written = 0;
            ok::Optional<ok::File::WriteError> err = state->file.write(record.buffer + col.offset, col.size, &n_written);
            OK_VERIFY(!err);
            OK_VERIFY(n_written == col.size);
        }

        break;
    }
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
