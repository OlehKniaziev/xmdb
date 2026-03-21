#pragma once

#include "ok.hpp"
#include "Result.hpp"

namespace xmdb {
using namespace ok::literals;

static inline constexpr U64 gen_magic(ok::StringView s) {
    U64 res = 0;

    for (UZ i = 0; i < ok::min(s.count, 8); ++i) {
        res = (res << 8) | (U64) s.data[i];
    }

    if (s.count < 8) res <<= 8 * (8 - s.count);

    return res;
}

constexpr U64 DB_STATE_HEADER_MAGIC = ((U64) 's' << 56) | ((U64) 'x' << 48) | ((U64) 'm' << 40) | ((U64) 'd' << 32) | ((U64) 'b' << 24);

/**
 * @brief Header for a table state file.
 */
struct DBStateHeader {
    U64 magic;        ///< Magic number identifying the file type.
    U64 record_count; ///< Total number of records in the table.
};

/**
 * @brief Represents the persistent state of a database table.
 */
struct DBState {
    DBStateHeader header; ///< The state header.
    ok::File file;        ///< The backing file for the state.
};

/**
 * @brief Creates a new table state in the given file.
 * @param file The file to use.
 * @param out Pointer to store the resulting DBState.
 * @return true if successful, false otherwise.
 */
bool db_state_create(ok::File file, DBState *out);

/**
 * @brief Tries to load a DBState from an open file.
 * @param allocator The allocator to use for error strings.
 * @param file The file to use.
 * @return A loaded DBState on success, an error string otherwise.
 */
Result<DBState, ok::String> db_state_load(ok::Allocator *allocator, ok::File file);

/**
 * @brief Synchronizes the table state to disk.
 * @param state Pointer to the state to sync.
 * @return true if successful, false otherwise.
 */
bool db_state_sync(DBState *state);

/**
 * @brief Resets the table state.
 * @param state Pointer to the state to reset.
 * @return true if successful, false otherwise.
 */
bool db_state_reset(DBState *state);

constexpr U64 COLUMN_STATE_HEADER_MAGIC = gen_magic("ixmdb"_sv);

/**
 * @brief Header for an image column state file.
 */
struct ImageColumnStateHeader {
    U64 magic;        ///< Magic number.
    U64 chunks_count; ///< Number of image chunks stored.
};

constexpr UZ IMAGE_COLUMN_STATE_HEADER_SIZE = sizeof(ImageColumnStateHeader);

/**
 * @brief Represents the persistent state of an image column.
 */
struct ImageColumnState {
    ImageColumnStateHeader header; ///< The state header.
    ok::File file;                 ///< The backing file.
};

/**
 * @brief Creates a new image column state in the given file.
 * @param file The file to use.
 * @param out Pointer to store the resulting ImageColumnState.
 * @return true if successful, false otherwise.
 */
bool image_state_create(ok::File file, ImageColumnState *out);

/**
 * @brief Loads an existing image column state from an open file.
 * @param allocator The allocator to use for error strings.
 * @param file The file to use.
 * @return Loaded ImageColumnState on success, an error string otherwise.
 */
Result<ImageColumnState, ok::String> image_state_load(ok::Allocator *allocator, ok::File file);

/**
 * @brief Synchronizes the image column state to disk.
 * @param state Pointer to the state to sync.
 * @return true if successful, false otherwise.
 */
bool image_state_sync(ImageColumnState *state);
} // namespace xmdb
