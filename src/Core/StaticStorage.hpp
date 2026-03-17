#pragma once

#include "ok.hpp"

namespace xmdb {
struct StaticStorage {
    ok::Table<ok::StringView, void *> builtin_functions;
};

StaticStorage *make_or_get_static_storage();
} // namespace xmdb
