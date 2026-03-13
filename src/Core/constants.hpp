#pragma once

#include "state.hpp"

#define KiB(x) (1024ull * (x))
#define MiB(x) (1024ull * KiB(x))
#define GiB(x) (1024ull * MiB(x))

namespace xmdb {
constexpr U64 BTREE_PAGE_SIZE = 4096;

constexpr U16 BTREE_HEADER_VERSION = 1;
constexpr U64 BTREE_HEADER_MAGIC = ((U64) 'x' << 56) | ((U64) 'm' << 48) | ((U64) 'd' << 40) | ((U64) 'b' << 32);
constexpr U64 BTREE_HEADER_LENGTH = BTREE_PAGE_SIZE;

constexpr U64 BTREE_ORDER = 128;

static_assert(BTREE_ORDER >= 2, "Order of the B-Tree is not allowed to be less than 2");

constexpr auto BTREE_MAX_KEYS = BTREE_ORDER * 2 - 1;
constexpr auto BTREE_MAX_CHILDREN = BTREE_ORDER * 2;

constexpr auto DB_STATE_HEADER_LENGTH = sizeof(DBStateHeader);
}
