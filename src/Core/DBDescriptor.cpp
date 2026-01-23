#include "DBDescriptor.hpp"
#include "new.hpp"
#include "Logger.hpp"
#include "SDFile.hpp"

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
    }

    tables.push(table);

    return table;
}
} // namespace xmdb
