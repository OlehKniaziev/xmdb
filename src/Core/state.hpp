#pragma once

#include "ok.hpp"

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

struct DBStateHeader {
    U64 magic;
    U64 record_count;
};

struct DBState {
    DBStateHeader header;
    ok::File file;
};

bool db_state_create(ok::File, DBState *);
bool db_state_sync  (DBState *);
bool db_state_reset (DBState *);

constexpr U64 COLUMN_STATE_HEADER_MAGIC = gen_magic("ixmdb"_sv);

struct ImageColumnStateHeader {
    U64 magic;
    U64 chunks_count;
};

constexpr UZ IMAGE_COLUMN_STATE_HEADER_SIZE = sizeof(ImageColumnStateHeader);

struct ImageColumnState {
    ImageColumnStateHeader header;
    ok::File file;
};

bool image_state_create(ok::File, ImageColumnState *);
bool image_state_sync  (ImageColumnState *);
} // namespace xmdb
