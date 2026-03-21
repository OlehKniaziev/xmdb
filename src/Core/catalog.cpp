#include "catalog.hpp"
#include "DBPool.hpp"
#include "DBDescriptor.hpp"
#include "FixedString.hpp"
#include "Logger.hpp"

namespace xmdb {
struct [[gnu::packed]] CatalogDBEntry {
    FixedString name;
    U64 user_count;
    U64 table_count;
};

struct [[gnu::packed]] CatalogUserEntry {
    FixedString name;
    U8 sha256_password_digest[SHA256Digest::SIZE];
    U8 perm;
};

struct [[gnu::packed]] CatalogTableEntry {
    FixedString name;
    DBTable::Flags flags;
    U64 column_count;
};

template <typename T>
static void write(ok::File *file, const T *value) {
    UZ n_written = 0;
    ok::Optional<ok::File::WriteError> write_err = file->write(reinterpret_cast<const U8 *>(value),
                                                               sizeof(*value),
                                                               &n_written);
    OK_VERIFY(!write_err);
    OK_VERIFY(n_written == sizeof(*value));
}

Result<UZ, ok::String> catalog_save(ok::Allocator *allocator,
                                    DBPool *pool,
                                    ok::StringView filename) {
    ok::File file{};
    ok::Optional<ok::File::OpenError> open_err = ok::File::open(&file, filename);
    if (open_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), open_err.get());
        return ok::String::format(allocator, "Failed to open catalog file for writing: %s", error_desc.cstr());
        return false;
    }

    U64 db_count = 0;
    for (DBDescriptor *db = pool->db_descriptors; db != nullptr; db = db->next) {
        ++db_count;
    }

    CatalogHeader header = {
        .magic = CATALOG_HEADER_MAGIC,
        .version = CATALOG_VERSION,
        .db_count = db_count,
    };

    file.seek_start();

    write(&file, &header);

    for (DBDescriptor *db = pool->db_descriptors; db != nullptr; db = db->next) {
        U64 user_count = 0;
        for (DBUser *user = db->users; user != nullptr; user = user->next) {
            ++user_count;
        }

        CatalogDBEntry entry = {
            .name = create_fixed_string(db->name),
            .user_count = user_count,
            .table_count = db->tables.count,
        };

        write(&file, &entry);

        for (DBUser *user = db->users; user != nullptr; user = user->next) {
            CatalogUserEntry user_entry = {
                .name = create_fixed_string(user->name),
                .sha256_password_digest = {},
                .perm = user->perm,
            };

            memcpy(&user_entry.sha256_password_digest,
                   user->sha256_password_digest.bytes.items,
                   sizeof(user_entry.sha256_password_digest));

            write(&file, &user_entry);
        }

        for (UZ table_idx = 0; table_idx < db->tables.count; ++table_idx) {
            DBTable *table = db->tables[table_idx];

            CatalogTableEntry table_entry = {
                .name = create_fixed_string(table->name()),
                .flags = table->flags(),
                .column_count = table->columns_count(),
            };

            write(&file, &table_entry);

            ok::Slice<ok::StringView> col_names = table->columns_names();
            ok::Slice<SQL::ColumnType> col_types = table->columns_types();

            for (UZ col_idx = 0; col_idx < table->columns_count(); ++col_idx) {
                ok::StringView col_name = col_names[col_idx];
                FixedString raw_col_name = create_fixed_string(col_name);
                write(&file, &raw_col_name);

                SQL::ColumnType col_type = col_types[col_idx];
                U8 raw_col_type = static_cast<U8>(col_type);
                write(&file, &raw_col_type);
            }
        }
    }

    file.close();

    return file.offset;
}

template <typename T>
static void read(ok::File *file, T *out) {
    UZ n_read;
    ok::Optional<ok::File::ReadError> err = file->read(reinterpret_cast<U8 *>(out), sizeof(*out), &n_read);
    OK_VERIFY(!err);
    OK_VERIFY(n_read == sizeof(*out));
}

Result<UZ, ok::String> catalog_load(ok::Allocator *allocator, DBPool *pool, ok::StringView filename) {
    ok::File file{};
    ok::Optional<ok::File::OpenError> open_err = ok::File::open(&file, filename);
    if (open_err) {
        return false;
    }

    file.seek_start();

    CatalogHeader header{};
    read(&file, &header);

    if (header.magic != CATALOG_HEADER_MAGIC) {
        file.close();
        return ok::String::alloc(allocator, "Invalid catalog file magic number");
    }

    if (header.version != CATALOG_VERSION) {
        file.close();
        return ok::String::format(allocator, "Unsupported catalog version %lu", header.version);
    }

    pool->db_descriptors = nullptr;

    for (U64 db_idx = 0; db_idx < header.db_count; ++db_idx) {
        CatalogDBEntry db_entry{};
        read(&file, &db_entry);

        DBDescriptor *db = pool->create_db(db_entry.name.view().to_string(allocator).view());

        for (U64 user_idx = 0; user_idx < db_entry.user_count; ++user_idx) {
            CatalogUserEntry user_entry{};
            read(&file, &user_entry);

            DBUser *user = allocator->alloc<DBUser>();
            user->next = nullptr;
            user->name = user_entry.name.view().to_string(allocator).view();
            user->perm = user_entry.perm;

            memcpy(user->sha256_password_digest.bytes.items,
                   user_entry.sha256_password_digest,
                   sizeof(user_entry.sha256_password_digest));

            db->add_user(user);
        }

        for (U64 table_idx = 0; table_idx < db_entry.table_count; ++table_idx) {
            CatalogTableEntry table_entry{};
            read(&file, &table_entry);

            ok::Slice<ok::StringView> column_names = allocator->alloc_slice<ok::StringView>(table_entry.column_count);
            ok::Slice<SQL::ColumnType> column_types = allocator->alloc_slice<SQL::ColumnType>(table_entry.column_count);

            for (U64 col_idx = 0; col_idx < table_entry.column_count; ++col_idx) {
                FixedString col_name_fs{};
                read(&file, &col_name_fs);

                U8 col_type_raw = 0;
                read(&file, &col_type_raw);

                column_names[col_idx] = col_name_fs.view();
                column_types[col_idx] = static_cast<SQL::ColumnType>(col_type_raw);
            }

            ok::StringView table_name = table_entry.name.view().to_string(allocator).view();

            Result<DBTable *, ok::String> load_table_result = db->load_existing_table(allocator,
                                                                                      table_name,
                                                                                      table_entry.flags,
                                                                                      (UZ) table_entry.column_count,
                                                                                      column_names,
                                                                                      column_types);

            if (!load_table_result.ok()) {
                return load_table_result.error();
            }
        }
    }

    file.close();

    return file.offset;
}
} // namespace xmdb
