#include "state.hpp"
#include "Logger.hpp"
#include "Result.hpp"

namespace xmdb {
bool db_state_create(ok::File file, DBState *out_state) {
    DBStateHeader header = {
        .magic = DB_STATE_HEADER_MAGIC,
        .record_count = 0,
    };

    file.seek_start();

    UZ n_written = 0;
    ok::Optional<ok::File::WriteError> write_err = file.write(reinterpret_cast<U8 *>(&header), sizeof(header), &n_written);
    if (write_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), write_err.get());
        log::error("Failed to write a header to the state file: %s",
                   error_desc.cstr());
        return false;
    }
    OK_VERIFY(n_written == sizeof(header));

    out_state->header = header;
    out_state->file = file;

    return true;
}

bool db_state_sync(DBState *state) {
    state->file.seek_start();

    UZ n_written = 0;
    ok::Optional<ok::File::WriteError> write_err = state->file.write(reinterpret_cast<U8 *>(&state->header), sizeof(state->header), &n_written);
    if (write_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), write_err.get());
        log::error("Failed to write a header to the state file during sync: %s",
                   error_desc.cstr());
        return false;
    }
    OK_VERIFY(n_written == sizeof(state->header));

    return true;
}

bool db_state_reset(DBState *state) {
    state->header.record_count = 0;
    return db_state_sync(state);
}

Result<DBState, ok::String> db_state_load(ok::Allocator *allocator, ok::File file) {
    DBStateHeader header{};

    file.seek_start();

    UZ n_read = 0;
    ok::Optional<ok::File::ReadError> read_err = file.read(reinterpret_cast<U8 *>(&header), sizeof(header), &n_read);
    if (read_err) {
        ok::String error_desc = ok::File::error_string(allocator, read_err.get());
        return ok::String::format(allocator, "Failed to read state file header: %s", error_desc.cstr());
    }

    if (n_read != sizeof(header)) {
        return ok::String::alloc(allocator, "State file is too small to contain a valid header");
    }

    if (header.magic != DB_STATE_HEADER_MAGIC) {
        return ok::String::alloc(allocator, "Invalid state file magic number");
    }

    DBState state = {
        .header = header,
        .file = file,
    };

    return state;
}

bool image_state_create(ok::File file, ImageColumnState *out_state) {
    ImageColumnStateHeader header = {
        .magic = COLUMN_STATE_HEADER_MAGIC,
        .chunks_count = 0,
    };

    file.seek_start();

    UZ n_written = 0;
    ok::Optional<ok::File::WriteError> write_err = file.write(reinterpret_cast<U8 *>(&header), sizeof(header), &n_written);
    if (write_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), write_err.get());
        log::error("Failed to write a header to the state file: %s",
                   error_desc.cstr());
        return false;
    }
    OK_VERIFY(n_written == sizeof(header));

    out_state->header = header;
    out_state->file = file;

    return true;
}

Result<ImageColumnState, ok::String> image_state_load(ok::Allocator *allocator, ok::File file) {
    ImageColumnStateHeader header{};

    file.seek_start();

    UZ n_read = 0;
    ok::Optional<ok::File::ReadError> read_err = file.read(reinterpret_cast<U8 *>(&header), sizeof(header), &n_read);
    if (read_err) {
        ok::String error_desc = ok::File::error_string(allocator, read_err.get());
        return ok::String::format(allocator, "Failed to read image state file header: %s", error_desc.cstr());
    }

    if (n_read != sizeof(header)) {
        return ok::String::alloc(allocator, "Image state file is too small to contain a valid header");
    }

    if (header.magic != COLUMN_STATE_HEADER_MAGIC) {
        return ok::String::alloc(allocator, "Invalid image state file magic number");
    }

    ImageColumnState state = {
        .header = header,
        .file = file,
    };

    return state;
}

bool image_state_sync(ImageColumnState *state) {
    state->file.seek_start();

    UZ n_written = 0;
    ok::Optional<ok::File::WriteError> write_err = state->file.write(reinterpret_cast<U8 *>(&state->header), sizeof(state->header), &n_written);
    if (write_err) {
        ok::String error_desc = ok::File::error_string(ok::temp_allocator(), write_err.get());
        log::error("Failed to write a header to the state file during sync: %s",
                   error_desc.cstr());
        return false;
    }
    OK_VERIFY(n_written == sizeof(state->header));

    return true;
}
} // namespace xmdb
