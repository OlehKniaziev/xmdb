#pragma once

#include "DBTable.hpp"
#include "DBUser.hpp"

namespace xmdb {
struct DBDescriptor {
    static DBDescriptor *alloc(ok::Allocator *, ok::StringView);
    DBDescriptor *next;
    ok::StringView name;
    ok::List<DBUser> users;
    ok::List<DBTable *> tables;
};
};
