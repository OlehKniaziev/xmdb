#pragma once

#include "DBTable.hpp"
#include "DBUser.hpp"

namespace xmdb {
struct DBDescriptor {
    DBDescriptor(ok::StringView name, ok::Allocator *allocator);

    ok::StringView name;
    ok::List<DBUser> users;
    ok::List<DBTable *> tables;
};
};
