#include "state.hpp"
#include "Logger.hpp"

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
