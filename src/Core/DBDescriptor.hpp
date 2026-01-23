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

    DBTable *create_new_table(ok::Allocator *allocator,
                              ok::StringView name,
                              DBTable::Flags flags,
                              UZ columns_count,
                              ok::Slice<ok::StringView> column_names,
                              ok::Slice<SQL::ColumnType> column_types);

    ok::Optional<DBTable *> load_existing_table(ok::StringView);

    DBDescriptor *next;
    ok::StringView name;
    DBUser *users;
    ok::List<DBTable *> tables;
};
}
