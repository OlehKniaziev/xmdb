#pragma once

#include "ok.hpp"

namespace xmdb {
constexpr U64 DB_STATE_HEADER_MAGIC = ((U64) 's' << 56) | ((U64) 'x' << 48) | ((U64) 'm' << 40) | ((U64) 'd' << 32) | ((U64) 'b' << 24);

struct DBStateHeader {
    U64 magic;
    U64 record_count;
};

struct DBState {
    ok::File file;
    DBStateHeader header;
};

bool db_state_create(ok::File, DBState *);
bool db_state_sync  (DBState *);
bool db_state_reset (DBState *);
} // namespace xmdb
