#include "DBDescriptor.hpp"
#include "new.hpp"
#include "Logger.hpp"
#include "SDFile.hpp"
#include "state.hpp"

namespace xmdb {
DBDescriptor *DBDescriptor::alloc(ok::Allocator *allocator, ok::StringView name) {
    DBDescriptor *descriptor = allocator->alloc<DBDescriptor>();
    descriptor->name = name;
    descriptor->next = nullptr;
    descriptor->users = nullptr;
    descriptor->tables = ok::List<DBTable *>::alloc(allocator);
    return descriptor;
}

static bool create_file_based_on_table_flags(DBTable::Flags flags,
                                             ok::String filename,
                                             ok::File *out_file) {
    if (flags & DBTable::F_EPHEMERAL) {
        SDFile sd_file{};

        if (!sd_file_create(filename.view(), &sd_file)) {
            log::error("Tried to create a SD file '%s', since the EPHEMERAL flag was supplied, but failed",
                       filename.cstr());
            return false;
        }

        *out_file = ok::File::from_fd(sd_file.fd, filename.cstr());
    } else {
        ok::Optional<ok::File::OpenError> open_error = ok::File::open(out_file, filename.cstr());

        if (open_error) {
            ok::String error_desc = ok::File::error_string(ok::temp_allocator(), open_error.get());
            log::error("Failed to open file '%s': %s",
                       filename.cstr(),
                       error_desc.cstr());
            return false;
        }
    }

    return true;
}

DBTable *DBDescriptor::create_new_table(ok::Allocator *allocator,
                                        ok::StringView name,
                                        DBTable::Flags flags,
                                        UZ columns_count,
                                        ok::Slice<ok::StringView> column_names,
                                        ok::Slice<SQL::ColumnType> column_types) {
    DBTable *table = new (allocator) DBTable{
        allocator,
        flags,
        name,
        columns_count,
        column_names.items,
        column_types.items
    };

    if (flags & DBTable::F_PERSIST) {
        SHA256Digest table_name_digest = sha256_digest(name);
        ok::Slice<U8> table_name_slice = table_name_digest.bytes.slice();
        ok::String table_digest_hex = to_hex_string(ok::temp_allocator(), table_name_slice);
        ok::String table_file_name = ok::String::format(allocator, "%s.xdb", table_digest_hex.cstr());
        ok::String record_file_name = ok::String::format(allocator, "%s.sdb", table_digest_hex.cstr());

        ok::File table_index_file{};

        if (!create_file_based_on_table_flags(flags, table_file_name, &table_index_file)) {
            return nullptr;
        }

        BTreeIndex table_index = BTreeIndex::create(allocator, table_index_file);

        table->set_index(table_index);

        ok::File record_file{};

        if (!create_file_based_on_table_flags(flags, record_file_name, &record_file)) {
            return nullptr;
        }

        DBState db_state{};

        if (!db_state_create(record_file, &db_state)) {
            return nullptr;
        }

        table->set_state(db_state);

        ok::Slice<ColumnAttribute> attrs = table->column_attributes();

        for (UZ col_idx = 0; col_idx < columns_count; ++col_idx) {
            ColumnAttribute *attr = &attrs[col_idx];
            if (attr->flags & ColumnAttribute::F_IMAGE) {
                ok::Slice<ok::StringView> column_names = table->columns_names();
                ok::StringView column_name = column_names[col_idx];

                ok::String column_file_name = ok::String::format(ok::temp_allocator(), OK_SV_FMT "-" OK_SV_FMT ".idb", OK_SV_ARG(name), OK_SV_ARG(column_name));

                ok::File image_file{};
                if (!create_file_based_on_table_flags(flags, column_file_name, &image_file)) {
                    return nullptr;
                }

                if (!image_state_create(image_file, &attr->u.image_state)) {
                    return nullptr;
                }
            }
        }
    }

    tables.push(table);

    return table;
}

Result<DBTable *, ok::String> DBDescriptor::load_existing_table(ok::Allocator *allocator,
                                                                ok::StringView name,
                                                                DBTable::Flags flags,
                                                                UZ columns_count,
                                                                ok::Slice<ok::StringView> column_names,
                                                                ok::Slice<SQL::ColumnType> column_types) {
    OK_ASSERT((flags & DBTable::F_PROXY) == 0);
    OK_ASSERT((flags & DBTable::F_EPHEMERAL) == 0);

    SHA256Digest table_name_digest = sha256_digest(name);
    ok::Slice<U8> table_name_slice = table_name_digest.bytes.slice();
    ok::String table_digest_hex = to_hex_string(ok::temp_allocator(), table_name_slice);
    ok::String table_index_file_name = ok::String::format(allocator, "%s.xdb", table_digest_hex.cstr());
    ok::String table_state_file_name = ok::String::format(allocator, "%s.sdb", table_digest_hex.cstr());

    ok::File table_index_file{};

    ok::Optional<ok::File::OpenError> open_err = ok::File::open(&table_index_file, table_index_file_name.view());
    if (open_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), open_err.get());
        return ok::String::format(allocator,
                                  "Failed to open table index file '%s': %s",
                                  table_index_file_name.cstr(),
                                  error_desc.cstr());
    }

    BTreeIndex table_index = BTreeIndex::create(allocator, table_index_file);

    ok::File table_state_file{};

    open_err = ok::File::open(&table_state_file, table_state_file_name.view());
    if (open_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), open_err.get());
        return ok::String::format(allocator,
                                  "Failed to open table state file '%s': %s",
                                  table_state_file_name.cstr(),
                                  error_desc.cstr());
    }

    Result<DBState, ok::String> db_state_load_result = db_state_load(allocator, table_state_file);

    if (!db_state_load_result.ok()) {
        return ok::String::format(allocator,
                                  "Failed to load db state from a file: %s",
                                  db_state_load_result.error().cstr());
    }

    DBState db_state = db_state_load_result.unwrap();

    ok::List<ColumnAttribute> column_attributes = ok::List<ColumnAttribute>::alloc(allocator);

    for (UZ col_idx = 0; col_idx < columns_count; ++col_idx) {
        SQL::ColumnType column_type = column_types[col_idx];
        ColumnAttribute attr = get_attribute_for_column_type(column_type);

        if (attr.flags & ColumnAttribute::F_IMAGE) {
            ok::StringView column_name = column_names[col_idx];

            ok::String image_file_name = ok::String::format(ok::temp_allocator(), OK_SV_FMT "-" OK_SV_FMT ".idb", OK_SV_ARG(name), OK_SV_ARG(column_name));

            ok::File image_file{};
            open_err = ok::File::open(&image_file, image_file_name.view());
            if (open_err) {
                ok::String error_desc = ok::File::error_string(ok::temp_allocator(), open_err.get());
                return ok::String::format(allocator,
                                          "Failed to open an image column state file '%s': %s",
                                          image_file_name.cstr(),
                                          error_desc.cstr());
            }

            Result<ImageColumnState, ok::String> load_image_state_result = image_state_load(allocator,
                                                                                            image_file);
            if (!load_image_state_result.ok()) {
                return ok::String::format(allocator,
                                          "Failed to load an image column state from file '%s': %s",
                                          image_file_name.cstr(),
                                          load_image_state_result.error().cstr());
            }

            ImageColumnState &image_col_state = load_image_state_result.unwrap();
            attr.u.image_state = image_col_state;
        }

        column_attributes.push(attr);
    }

    DBTable *table = new (allocator) DBTable{
        allocator,
        flags,
        name,
        columns_count,
        column_names.items,
        column_types.items,
        column_attributes.items,
    };

    table->set_index(table_index);
    table->set_state(db_state);

    tables.push(table);

    return table;
}

} // namespace xmdb
