#pragma once

#include "ok.hpp"
#include "Value.hpp"

namespace xmdb {
struct DBRecord {
    U8 *buffer;
    UZ buffer_size;
    U64 primary_key_value;
};

void fill_column(DBRecord *, TableLayout, UZ, Value);

DBRecord db_record_create(ok::Allocator *, TableLayout);
} // namespace xmdb
