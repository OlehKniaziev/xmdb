#pragma once

#include "ok.hpp"
#include "state.hpp"

namespace xmdb
{
struct DBPool;

constexpr U64 CATALOG_HEADER_MAGIC = gen_magic("cxmdb"_sv);
constexpr U64 CATALOG_VERSION = 1;

struct CatalogHeader
{
    U64 magic;
    U64 version;
    U64 db_count;
};

Result<UZ, ok::String> catalog_save(ok::Allocator *allocator, DBPool *pool,
                                    ok::StringView filename);
Result<UZ, ok::String> catalog_load(ok::Allocator *allocator, DBPool *pool,
                                    ok::StringView filename);
} // namespace xmdb
