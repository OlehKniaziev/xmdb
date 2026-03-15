#include "QueryExecutionContext.hpp"
#include "new.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "Png.hpp"

using namespace ok::literals;

namespace xmdb {
#define X(p, _unused)                                                                                                  \
    void CHECK_##p(QueryExecutionContext *ctx) {                                                                       \
        if ((ctx->user->perm & PERM_##p) == 0) {                                                                       \
            XMDB_FAIL_FMT(ctx, "user " OK_SV_FMT " does not have " #p " permissions", OK_SV_ARG(ctx->user->name));          \
        }                                                                                                              \
    }

XMDB_ENUM_USER_PERMISSIONS

#undef X

DBValue *QueryExecutionContext::fetch_var(U32 ip) {
    CHECK_READ(this);
    Optional<DBValue *> value = vars.get(ip);
    return value.get();
}

void QueryExecutionContext::put_var(U32 ip, StringView name, DBValue *value) {
    OK_UNUSED(name);
    CHECK_WRITE(this);
    vars.put(ip, value);
}

DBValue *QueryExecutionContext::fetch_column(DBTable *table, StringView column_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < table->columns_count(); ++i) {
        if (table->columns_names()[i] == column_name) {
            ColumnLayout layout = table->layout().columns[i];
            SQL::ColumnType column_type = table->columns_types()[i];
            SQL::Type value_type = column_type_to_type(column_type);
            return new (allocator) ColumnDBValue{value_type, layout, table};
        }
    }

    OK_UNREACHABLE();
}

DBTable *QueryExecutionContext::fetch_table(StringView table_name) {
    CHECK_READ(this);

    for (UZ i = 0; i < current_db->tables.count; ++i) {
        DBTable *table = current_db->tables[i];
        if (table->name() == table_name) {
            return table;
        }
    }

    OK_UNREACHABLE();
}

void QueryExecutionContext::create_table(StringView table_name, SQL::TableSchema *table_schema) {
    CHECK_WRITE(this);

    if (!table_schema->columns_types.has_value()) {
        OK_TODO();
    }

    UZ columns_count = table_schema->columns_names.count;
    List<SQL::ColumnType> column_types = table_schema->columns_types.value;
    ok::StringView *column_names_ptr = allocator->alloc<ok::StringView>(columns_count);
    ok::Slice<ok::StringView> column_names = {column_names_ptr, columns_count};
    for (UZ i = 0; i < table_schema->columns_names.count; ++i) {
        if (table_schema->columns_names[i].has_value()) {
            column_names[i] = table_schema->columns_names[i].value.view();
        } else {
            column_names[i] = ""_sv;
        }
    }

    DBTable *new_table = current_db->create_new_table(allocator,
                                                      table_name,
                                                      DBTable::F_EPHEMERAL | DBTable::F_PERSIST,
                                                      column_types.count,
                                                      column_names,
                                                      column_types.slice());

    if (new_table == nullptr) {
        XMDB_FAIL_FMT(this,
                      "Failed to create table '" OK_SV_FMT "', see the log for details",
                      OK_SV_ARG(table_name));
    }
}

void QueryExecutionContext::emit_column(DBTable *table, DBValue *column_value, StringView column_name) {
    emitted_columns.push(column_name, column_value, table);
}

DBTable *QueryExecutionContext::emit_query(U32 columns_count, SQL::ColumnType *columns_types) {
    XMDB_FIXME("This *should* also be added to the query graph, since we want every operation to preserve it's order as in the source code");

    const StringView *columns_names_ptr = emitted_columns.get_items<StringView>();
    DBValue **const columns_values_ptr = emitted_columns.get_items<DBValue *>();
    DBTable **tables = emitted_columns.get_items<DBTable *>();

    StringView *columns_names = allocator->alloc<StringView>(columns_count);
    DBValue **columns_values = allocator->alloc<DBValue *>(columns_count);
    memcpy(columns_names, columns_names_ptr, columns_count * sizeof(*columns_names));
    memcpy(columns_values, columns_values_ptr, columns_count * sizeof(*columns_values));

    // NOTE(oleh): This is gonna be wrong once the row filtering is added.
    UZ rows_count = 0;

    for (UZ i = 0; i < emitted_columns.count; ++i) {
        DBTable *table = tables[i];
        if (table->rows_count() > rows_count) {
            rows_count = table->rows_count();
        }
    }

    emitted_columns.count = 0;

    DBTable *table = new (allocator) DBTable{allocator, DBTable::F_ANON | DBTable::F_PROXY, ""_sv, columns_count, columns_names, columns_types};

    Slice<DBValue *> columns_values_slice = {columns_values, columns_count};
    table->set_proxy_column_values(columns_values_slice);
    table->set_proxy_rows_count(rows_count);
    last_emitted_query = table;
    return table;
}

void QueryExecutionContext::insert_column(DBTable *table, StringView column_name, DBValue *column_value) {
    CHECK_WRITE(this);

    if (!table_to_insert) {
        table_to_insert = table;
    } else {
        OK_ASSERT(table == table_to_insert.get());
    }

    ok::Optional<ok::List<DBValue *>> values_opt = rows_to_insert.get(column_name);
    ok::List<DBValue *> values{};

    if (values_opt) {
        values = values_opt.get();
    } else {
        values = ok::List<DBValue *>::alloc(allocator);
    }

    values.push(column_value);
    rows_to_insert.put(column_name, values);
}

void QueryExecutionContext::insert_row() {
    OK_ASSERT(table_to_insert);
    ++rows_to_insert_count;
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

void QueryExecutionContext::fill_column(DBRecord *record,
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
        ColumnAttribute attr = attrs[column_index];

        OK_ASSERT(attr.flags & ColumnAttribute::F_IMAGE);

        ImageColumnState *image_state = &attr.u.image_state;

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
    }
}

void QueryExecutionContext::commit_insert() {
    CHECK_WRITE(this);

    OK_ASSERT(table_to_insert);

    DBTable *table = table_to_insert.get();

    if (table->flags() & DBTable::F_PERSIST) {
        TableLayout table_layout = table->layout();

        UZ record_values_count = table->columns_count();
        Slice<DBValue *> *record_items = allocator->alloc<Slice<DBValue *>>(record_values_count);
        Slice<Slice<DBValue *>> record_values = {record_items, record_values_count};

        OK_TABLE_FOREACH(rows_to_insert, column_name, column_values, {
            UZ column_index = table->columns_names().find_index(column_name);
            OK_ASSERT(column_index != (UZ)-1);

            record_values[column_index] = column_values.slice();
        });

        DBRecord *records_items = allocator->alloc<DBRecord>(DB_RECORD_BUFFER_CAPACITY);
        Slice<DBRecord> records_buffer = {records_items, DB_RECORD_BUFFER_CAPACITY};
        UZ records_buffer_count = 0;

        for (UZ row_index = 0; row_index < rows_to_insert_count; ++row_index) {
            DBRecord record = db_record_create(allocator, table_layout);

            for (UZ column_index = 0; column_index < record_values.count; ++column_index) {
                Slice<DBValue *> column_values = record_values[column_index];
                DBValue *column_db_value = column_values[row_index];
                ok::Optional<Value> column_value = poll(allocator, column_db_value);
                OK_VERIFY(column_value);
                fill_column(&record, table, column_index, column_value.get());
            }

            if (records_buffer_count >= records_buffer.count) {
                OK_PANIC("break");
                flush_buffer(this, table, records_buffer);
                records_buffer_count = 0;
            } else {
                records_buffer[records_buffer_count] = record;
                ++records_buffer_count;
            }
        }

        flush_buffer(this, table, records_buffer.slice(0, records_buffer_count));
    } else {
        OK_TODO();
#if 0
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

    rows_to_insert.clear();
}

void QueryExecutionContext::update_column(DBTable *table, StringView column_name, DBValue *column_value) {
    if (!table_to_update.has_value()) {
        table_to_update = table;
    } else if (table_to_update.value != table) {
        OK_PANIC("Table to update does not match the given table");
    }

    columns_to_update.push(column_name, column_value);
}

void QueryExecutionContext::commit_update() {
    CHECK_WRITE(this);

    DBTable *table = table_to_update.get();
    StringView *columns_to_update_names = columns_to_update.get_items<StringView>();
    DBValue **columns_to_update_values = columns_to_update.get_items<DBValue *>();

    if (table->flags() & DBTable::F_PERSIST) {
        Slice<StringView> column_names = table->columns_names();

        for (UZ uc = 0; uc < columns_to_update.count; ++uc) {
            StringView column_name = columns_to_update_names[uc];
            UZ idx = column_names.find_index(column_name);
            OK_ASSERT(idx != (U64) -1);

            query_graph.write_column(table, idx, columns_to_update_values[uc]);
        }
    } else {
        Slice<DBValue *> values = table->proxy_column_values();

        for (UZ i = 0; i < columns_to_update.count; ++i) {
            StringView column_name = columns_to_update_names[i];
            for (UZ c = 0; c < table->columns_count(); ++c) {
                if (table->columns_names()[c] != column_name) continue;
                DBValue *new_value = columns_to_update_values[i];
                values[c] = new_value;
                break;
            }
        }
    }

    columns_to_update.count = 0;
    table_to_update = {};
}

void QueryExecutionContext::delete_table(DBTable *table) {
    CHECK_WRITE(this);

    if (table->flags() & DBTable::F_PROXY) {
        ok::Slice<DBValue *> values = table->proxy_column_values();

        for (UZ i = 0; i < table->columns_count(); ++i) {
            DBValue *new_value = new (allocator) NoneDBValue{};
            values[i] = new_value;
        }

        table->set_proxy_rows_count(0);
    } else {
        BTreeIndex *index = table->index();
        OK_VERIFY(index->reset());

        DBState *state = table->state();
        OK_VERIFY(db_state_reset(state));
    }
}

void QueryExecutionContext::create_user(StringView name) {
    DBUser *user = allocator->alloc<DBUser>();
    *user = {name, ""_sv, PERM_READ | PERM_WRITE};
    current_db->add_user(user);
}

void QueryExecutionContext::alter_user_property(StringView user_name, StringView property_name, DBValue *value) {
    CHECK_ADMIN(this);

    if (!alter_user_atomic_node) {
        alter_user_atomic_node = query_graph.atomic();
    }

    QueryGraph::AtomicNode *atomic_node = alter_user_atomic_node.get();

    if (property_name == "PASSWORD"_sv) {
        if (value->type() != SQL::TYPE_STRING)
            XMDB_FAIL_FMT(this, "expected user password to be of type string, got type '%s' instead",
                     SQL::type_name(value->type()));

        auto *alter_node = new (allocator) QueryGraph::AlterUserNode{user_name, QueryGraph::AlterUserNode::Property::PASSWORD, value};
        atomic_node->add(alter_node);
    } else {
        XMDB_FAIL_FMT(this, "user '" OK_SV_FMT "' does not have property '" OK_SV_FMT "'", OK_SV_ARG(user_name),
                 OK_SV_ARG(property_name));
    }
}

void QueryExecutionContext::commit_alter_user() {
    alter_user_atomic_node = {};
}

DBValue *QueryExecutionContext::compare(DBValue *lhs, DBValue *rhs) {
    return new (allocator) CompareDBValue{lhs, rhs};
}

DBTable *QueryExecutionContext::fetch_table(U32 ip) {
    return tables.get(ip).get();
}

void QueryExecutionContext::put_table(U32 ip, DBTable *table) {
    tables.put(ip, table);
}

void QueryExecutionContext::prepare_call_arg(DBValue *arg_value) {
    call_args.push(arg_value);
}

void QueryExecutionContext::call(ok::StringView fn_name, U64 arg_count) {
    OK_ASSERT(call_args.count == arg_count);

    Slice<DBValue *> args = call_args.slice();

    query_graph.call(fn_name, args);

    call_args.count = 0;
}
} // namespace xmdb
