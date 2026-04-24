#pragma once

#include <Core/ok.hpp>

namespace xmdb::plugin
{
static inline char *sv_to_cstr(ok::Allocator *allocator, ok::StringView sv)
{
    char *cstr = allocator->alloc<char>(sv.count + 1);
    memcpy(cstr, sv.data, sv.count);
    cstr[sv.count] = '\0';
    return cstr;
}
} // namespace xmdb::plugin
