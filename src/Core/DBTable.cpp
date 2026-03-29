#include "DBTable.hpp"
#include "FixedString.hpp"
#include "Logger.hpp"
#include "Png.hpp"
#include "constants.hpp"
#include "new.hpp"

using namespace ok::literals;

namespace xmdb {
ColumnAttribute get_attribute_for_column_type(SQL::ColumnType column_type) {
    if (column_type == SQL::ColumnType::PNG) {
        return ColumnAttribute{
                .flags = ColumnAttribute::F_IMAGE,
                .u =
                        {
                                .image_state = {},
                        },
        };
    } else {
        return {};
    }
}

struct TypeLayout {
    static constexpr UZ MAX_ALIGNMENT = sizeof(UZ);

    UZ size;
    UZ alignment;
};

static TypeLayout type_layout(SQL::ColumnType type) {
    static_assert(sizeof(FixedString) % 8 == 0);

    switch (type) {
    case SQL::ColumnType::INTEGER: return {.size = 8, .alignment = 8};
    case SQL::ColumnType::FLOAT:   return {.size = 4, .alignment = 4};
    case SQL::ColumnType::DOUBLE:  return {.size = 8, .alignment = 8};
    case SQL::ColumnType::BOOLEAN: return {.size = 1, .alignment = 1};
    case SQL::ColumnType::TEXT:
        return {.size = sizeof(FixedString), .alignment = 8};
    case SQL::ColumnType::PNG:   return {.size = sizeof(Png), .alignment = 8};
    case SQL::ColumnType::MEDIA: OK_TODO();
    }

    OK_UNREACHABLE();
}

// TODO(oleh): Reorder columns so that disk layout is optimized. Usually this
// would involve sorting the columns by type sizes in ascending order.
static ColumnLayout *compute_columns_layout(ok::Allocator *allocator,
                                            UZ columns_count,
                                            SQL::ColumnType *columns_types) {
    ColumnLayout *columns_layout =
            allocator->alloc<ColumnLayout>(columns_count);
    UZ offset = 0;

    for (UZ i = 0; i < columns_count; ++i) {
        SQL::ColumnType column_type = columns_types[i];
        TypeLayout t_layout = type_layout(column_type);

        if (t_layout.alignment != 1) {
            OK_ASSERT((t_layout.alignment & 1) == 0);

            offset = ok::align_up(offset, t_layout.alignment);
        }

        ColumnLayout column_layout = {
                .index = i,
                .offset = offset,
                .size = t_layout.size,
        };

        columns_layout[i] = column_layout;

        offset += t_layout.size;
    }

    return columns_layout;
}

DBTable::DBTable(ok::Allocator *allocator, DBTable::Flags flags,
                 ok::StringView name, UZ columns_count,
                 ok::StringView *columns_names,
                 SQL::ColumnType *columns_types) :
    m_name{name}, m_flags{flags}, m_columns_count{columns_count},
    m_columns_names{columns_names}, m_columns_types{columns_types} {
    ColumnLayout *columns_layout =
            compute_columns_layout(allocator, columns_count, columns_types);

    UZ id_column_index = (UZ) -1;
    for (UZ i = 0; i < columns_count; ++i) {
        if (columns_names[i] == "id"_sv) {
            id_column_index = i;
            break;
        }
    }

    if (id_column_index == (UZ) -1) {
        OK_VERIFY(columns_count > 0);
        id_column_index = 0;
    }

    XMDB_FIXME("DBTable constructor searches for a column with name 'id' and "
               "sets it as a primary key column");

    m_layout.primary_key_index = id_column_index;
    m_layout.columns.items = columns_layout;
    m_layout.columns.count = columns_count;

    if (flags & F_PROXY) {
        OK_ASSERT(flags & F_ANON);

        m_proxy_columns_values = allocator->alloc<DBValue *>(m_columns_count);
        for (UZ i = 0; i < m_columns_count; ++i) {
            m_proxy_columns_values[i] = new (allocator) NoneDBValue{};
        }
    }

    if (flags & F_EPHEMERAL) {
        OK_ASSERT(flags & F_PERSIST);
    }

    m_column_attributes = allocator->alloc<ColumnAttribute>(m_columns_count);

    for (UZ i = 0; i < m_columns_count; ++i) {
        ColumnAttribute attribute{};

        SQL::Type ty = SQL::column_type_to_type(m_columns_types[i]);

        if (is_image(ty)) {
            attribute.flags |= ColumnAttribute::F_IMAGE;
        }

        m_column_attributes[i] = attribute;
    }
}

DBTable::DBTable(ok::Allocator *allocator, DBTable::Flags flags,
                 ok::StringView name, UZ columns_count,
                 ok::StringView *columns_names, SQL::ColumnType *columns_types,
                 ColumnAttribute *column_attributes) :
    m_name{name}, m_flags{flags}, m_columns_count{columns_count},
    m_columns_names{columns_names}, m_columns_types{columns_types} {
    ColumnLayout *columns_layout =
            compute_columns_layout(allocator, columns_count, columns_types);

    UZ id_column_index = (UZ) -1;
    for (UZ col_idx = 0; col_idx < columns_count; ++col_idx) {
        if (columns_names[col_idx] == "id"_sv) {
            id_column_index = col_idx;
            break;
        }
    }

    if (id_column_index == (UZ) -1) {
        OK_VERIFY(columns_count > 0);
        id_column_index = 0;
    }

    XMDB_FIXME("DBTable constructor searches for a column with name 'id' and "
               "sets it as a primary key column");

    m_layout.primary_key_index = id_column_index;
    m_layout.columns.items = columns_layout;
    m_layout.columns.count = columns_count;

    if (flags & F_PROXY) {
        OK_ASSERT(flags & F_ANON);

        m_proxy_columns_values = allocator->alloc<DBValue *>(m_columns_count);
        for (UZ col_idx = 0; col_idx < m_columns_count; ++col_idx) {
            m_proxy_columns_values[col_idx] = new (allocator) NoneDBValue{};
        }
    }

    if (flags & F_EPHEMERAL) {
        OK_ASSERT(flags & F_PERSIST);
    }

    for (UZ col_idx = 0; col_idx < m_columns_count; ++col_idx) {
        SQL::Type ty = SQL::column_type_to_type(m_columns_types[col_idx]);
        bool type_is_image = is_image(ty);
        bool attr_is_image =
                column_attributes[col_idx].flags & ColumnAttribute::F_IMAGE;
        OK_VERIFY(type_is_image == attr_is_image);
    }

    m_column_attributes = column_attributes;
}

struct StreamPair {
    DBTableStream lhs;
    DBTableStream rhs;
};

constexpr bool chunk_images = false;

static constexpr const char *PULL_NAME = "pull";

static Result<Pipeline *, ok::String>
create_demux_pull_pipeline(ok::Allocator *allocator, MediaSource source) {
    MediaSourceFormat source_format = CHECK(source.identify_format());

    VideoPlugin *plugin = CHECK(video_plugin_for(source_format));

    Pipeline *pipeline = CHECK(Pipeline::create(allocator, plugin, nullptr));

    auto *pull = new (allocator)
            Pull{[](Pull *pull, VideoFrame *frame, void *data) {
                     (void) pull;
                     (void) frame;
                     (void) data;
                     OK_TODO_MSG("Actual streaming should be done here");
                 },
                 nullptr};

    auto *demux = new (allocator) Demux{source};
    demux->on_new_stream(
            [](Demux *demux, MediaStream *stream, void *data) {
                auto *pull = reinterpret_cast<Pull *>(data);
                ok::Optional<ok::String> connect_err =
                        demux->pipeline->connect(stream, pull);
                if (connect_err) {
                    OK_PANIC_FMT("Failed to connect pipeline elements: %s",
                                 connect_err.get().cstr());
                }
            },
            pull);

    pipeline->add(demux, "demux");
    pipeline->add(pull, PULL_NAME);

    return pipeline;
}

DBTableStream DBTableStream::from_value(ok::Allocator *allocator,
                                        DBValue *value) {
    switch (value->kind()) {
    case DBValue::Kind::DELAYED: {
        auto delayed = static_cast<DelayedDBValue *>(value);
        ok::Optional<DBValue *> delayed_value_opt = delayed->value();

        if (!delayed_value_opt.has_value()) {
            return DBTableStream{
                    allocator,
                    [](void *arg) -> ok::Optional<Value> {
                        (void) arg;
                        return ok::Optional<Value>::empty();
                    },
                    nullptr,
            };
        } else {
            DBValue *delayed_value = delayed_value_opt.get();
            return DBTableStream::from_value(allocator, delayed_value);
        }
    }
    case DBValue::Kind::CONSTANT: {
        auto *const_val = static_cast<ConstDBValue *>(value);
        return DBTableStream{allocator, *const_val};
    }
    case DBValue::Kind::COMPARE: {
        auto *cmp_val = static_cast<CompareDBValue *>(value);

        DBTableStream lhs = from_value(allocator, cmp_val->lhs());
        DBTableStream rhs = from_value(allocator, cmp_val->rhs());

        auto *cmp_state = new (allocator) StreamPair{lhs, rhs};

        return DBTableStream{
                allocator,
                [](void *arg) -> ok::Optional<Value> {
                    auto *state = static_cast<StreamPair *>(arg);
                    ok::Optional<Value> lhs_opt = state->lhs.next();
                    ok::Optional<Value> rhs_opt = state->rhs.next();

                    if (lhs_opt && rhs_opt) {
                        Value lhs = lhs_opt.get();
                        Value rhs = rhs_opt.get();
                        return lhs.compare(rhs);
                    }

                    if (lhs_opt) {
                        return Value::greater();
                    }

                    if (rhs_opt) {
                        return Value::less();
                    }

                    return ok::Optional<Value>::empty();
                },
                static_cast<void *>(cmp_state),
        };
    }
    case DBValue::Kind::NONE: {
        return DBTableStream{
                allocator,
                [](void *arg) -> ok::Optional<Value> {
                    OK_UNUSED(arg);
                    return ok::Optional<Value>::empty();
                },
                nullptr,
        };
    }
    case DBValue::Kind::CONCAT: {
        auto *concat_value = static_cast<ConcatDBValue *>(value);

        DBTableStream lhs = from_value(allocator, concat_value->lhs());
        DBTableStream rhs = from_value(allocator, concat_value->rhs());

        auto *stream_pair = new (allocator) StreamPair{lhs, rhs};

        return DBTableStream{
                allocator,
                [](void *arg) -> ok::Optional<Value> {
                    auto *pair = reinterpret_cast<StreamPair *>(arg);

                    if (ok::Optional<Value> lhs = pair->lhs.next(); lhs) {
                        return lhs;
                    } else {
                        return pair->rhs.next();
                    }
                },
                stream_pair,
        };
    }
    case DBValue::Kind::COLUMN: {
        auto *column_value = static_cast<ColumnDBValue *>(value);
        DBTable *table = column_value->table();
        ColumnLayout column_layout = column_value->layout();

        if (table->flags() & DBTable::F_PROXY) {
            ok::Slice<DBValue *> columns_values = table->proxy_column_values();

            DBValue *value = columns_values[column_layout.index];

            return DBTableStream::from_value(allocator, value);
        } else {
            OK_ASSERT(table->flags() & DBTable::F_PERSIST);

            return DBTableStream{
                    allocator,
                    table,
                    column_layout.index,
            };
        }
    }
    case DBValue::Kind::IMAGE_DATA: {
        auto *image_value = static_cast<ImageDataDBValue *>(value);

        if (chunk_images) OK_TODO_MSG("Image chunking");

        struct State {
            ok::Allocator *allocator;
            ImageDataDBValue *img;
            UZ times;
        };

        auto *state = allocator->alloc<State>();
        state->allocator = allocator;
        state->img = image_value;
        state->times = 0;

        return DBTableStream{
                allocator,
                [](void *arg) -> ok::Optional<Value> {
                    auto *state = static_cast<State *>(arg);

                    if (state->times > 0) return ok::Optional<Value>::empty();

                    ImageDataDBValue *img = state->img;
                    ++state->times;

                    return Value::image_chunk(state->allocator, 0, 0,
                                              img->width(), img->height(),
                                              img->data(), img->format());
                },
                state,
        };
    }
    case DBValue::Kind::MEDIA: {
        auto *media_value = static_cast<MediaSourceDBValue *>(value);
        MediaSource media_source = media_value->source();
        Result<Pipeline *, ok::String> pipeline_res =
                create_demux_pull_pipeline(allocator, media_source);

        if (!pipeline_res.ok()) {
            OK_PANIC_FMT("Failed to create a pipeline: %s",
                         pipeline_res.error().cstr());
        }

        auto *pipeline = pipeline_res.unwrap();

        return DBTableStream{
                allocator,
                [](void *data) -> ok::Optional<Value> {
                    auto *pipeline = static_cast<Pipeline *>(data);
                    Result<PipelineElement *, ok::String> pull_res =
                            pipeline->get_element(PULL_NAME);
                    if (!pull_res.ok()) {
                        OK_PANIC_FMT("Could not get the pull element: %s",
                                     pull_res.error().cstr());
                    }

                    auto *pull = static_cast<Pull *>(pull_res.unwrap());
                    OK_TODO_MSG("Dunno what to do here tbh!");
                    (void) pull;
                },
                pipeline,
        };

        break;
    }
    }

    OK_UNREACHABLE();
}

ok::Optional<Value> DBTableStream::next() {
    switch (m_type) {
    case Type::CONSTANT: {
        ConstDBValue constant = m_u.constant;

        switch (constant.kind()) {
        case ConstDBValue::ConstKind::INT: {
            S64 value = constant.as_int();
            return Value::integer(value);
        }
        case ConstDBValue::ConstKind::BOOL: {
            bool value = constant.as_bool();
            return Value::boolean(value);
        }
        case ConstDBValue::ConstKind::STRING: {
            ok::String *value = constant.as_string();
            if (value->count() <= FixedString::DATA_SIZE) {
                FixedString fs{};
                memcpy(fs.items, (U8 *) value->cstr(), value->count());
                fs.count = (U8) value->count();
                return Value::string(m_allocator, fs);
            } else {
                XMDB_FIXME("eliminate copying of big strings");
                return Value::big_string(m_allocator, value->view());
            }
        }
        }

        OK_UNREACHABLE();
    }
    case Type::COMPUTE: {
        ComputationStream comp = m_u.compute;
        return comp.comp(comp.arg);
    }
    case Type::COLUMN: {
        ColumnStream *col = &m_u.column;
        DBTable *table = col->table;
        DBState *state = table->state();
        UZ records_count = state->header.record_count;

        if (col->current_record >= records_count) {
            return ok::Optional<Value>::empty();
        }

        TableLayout layout = table->layout();
        ColumnLayout target_column = layout.columns[col->column_index];

        UZ record_size = table_record_size(layout);

        state->file.seek_to(DB_STATE_HEADER_LENGTH +
                            col->current_record * record_size +
                            target_column.offset);

        UZ n_read = 0;

        ok::Optional<ok::File::ReadError> read_err =
                state->file.read(col->buffer, target_column.size, &n_read);
        OK_VERIFY(!read_err);
        OK_VERIFY(n_read == target_column.size);

        SQL::ColumnType column_type = table->columns_types()[col->column_index];

        ++col->current_record;

        switch (column_type) {
        case SQL::ColumnType::INTEGER: {
            U64 result_u = 0;

            for (SZ i = 0; i < 7; ++i) {
                result_u = (result_u | (U64) col->buffer[i]) << 8;
            }

            result_u |= (U64) col->buffer[7];

            S64 result = (S64) result_u;

            return Value::integer(result);
        }
        case SQL::ColumnType::TEXT: {
            FixedString result{};
            memcpy(&result, col->buffer, sizeof(FixedString));
            return Value::string(m_allocator, result);
        }
        case SQL::ColumnType::PNG: {
            Png *png = reinterpret_cast<Png *>(col->buffer);
            OK_VERIFY(png->indices.count == 1);

            U8 index = png->indices.items[0];

            ok::Slice<ColumnAttribute> attrs = table->column_attributes();
            ColumnAttribute attr = attrs[col->column_index];

            OK_ASSERT(attr.flags & ColumnAttribute::F_IMAGE);

            ImageColumnState image_state = attr.u.image_state;

            image_state.file.seek_to(IMAGE_COLUMN_STATE_HEADER_SIZE +
                                     MAX_DISK_IMAGE_CHUNK_SIZE * index);

            UZ n_read = 0;
            UZ chunk_size = MAX_DISK_IMAGE_CHUNK_SIZE;
            U8 *chunk_buffer = m_allocator->alloc<U8>(chunk_size);
            ok::Optional<ok::File::ReadError> err =
                    image_state.file.read(chunk_buffer, chunk_size, &n_read);
            OK_VERIFY(!err);
            OK_VERIFY(n_read == chunk_size);

            DiskImageChunk *disk_chunk =
                    reinterpret_cast<DiskImageChunk *>(chunk_buffer);
            OK_ASSERT(disk_chunk->width == png->header.width);
            OK_ASSERT(disk_chunk->height == png->header.height);

            U8 pixel_size = format_pixel_size_in_bytes(png->format);

            ok::Slice<U8> pixel_data = {disk_chunk->pixel_data,
                                        disk_chunk->width * disk_chunk->height *
                                                pixel_size};

            return Value::image_chunk(m_allocator, disk_chunk->x, disk_chunk->y,
                                      disk_chunk->width, disk_chunk->height,
                                      pixel_data, png->format);
        }
        case SQL::ColumnType::MEDIA: {
            // Result<Pipeline *, ok::String> pipeline_res =
            // create_demux_pull_pipeline(m_allocator, media_source);
            OK_TODO_MSG("media column");
        }
        case SQL::ColumnType::BOOLEAN: OK_TODO_MSG("[next] BOOL");
        case SQL::ColumnType::FLOAT:   OK_TODO_MSG("[next] FLOAT");
        case SQL::ColumnType::DOUBLE:  OK_TODO_MSG("[next] DOUBLE");
        }

        OK_UNREACHABLE();
    }
    }

    OK_UNREACHABLE();
}

Value Value::compare(Value) {
    OK_TODO();
}
} // namespace xmdb
