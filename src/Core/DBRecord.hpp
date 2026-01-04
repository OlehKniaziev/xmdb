#pragma once

#include "ok.hpp"

namespace xmdb {
struct DBRecord {
    U64 m_size;
    U8 m_data[];
};
} // namespace xmdb
