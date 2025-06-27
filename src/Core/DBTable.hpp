#pragma once

#include "DBValue.hpp"
#include "DBIndex.hpp"
#include "ok.hpp"

namespace xmdb {
struct DBTable {
    ok::StringView name;
    UZ column_count;
    ok::StringView *column_names;
    DBValue *column_values;
    ok::Table<UZ, DBIndex> indices;
};
};
