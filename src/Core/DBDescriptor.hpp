#pragma once

#include "DBTable.hpp"
#include "DBUser.hpp"

namespace xmdb {
struct DBDescriptor {
    static DBDescriptor *alloc(ok::Allocator *, ok::StringView);

    inline void add_user(DBUser *user) {
        user->next = users;
        users = user;
    }

    DBDescriptor *next;
    ok::StringView name;
    DBUser *users;
    ok::List<DBTable *> tables;
};
};
