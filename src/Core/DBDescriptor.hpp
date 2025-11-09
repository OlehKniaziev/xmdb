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

    inline Optional<DBUser *> find_user(StringView name) {
        for (DBUser *user = users; user != nullptr; user = user->next) {
            if (user->name == name) {
                return user;
            }
        }

        return {};
    }

    DBDescriptor *next;
    ok::StringView name;
    DBUser *users;
    ok::List<DBTable *> tables;
};
};
