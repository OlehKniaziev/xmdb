#pragma once

#include "ok.hpp"

using namespace ok::literals;

namespace xmdb {

#define XMDB_ENUM_USER_PERMISSIONS                                                                                     \
    X(READ)                                                                                                            \
    X(WRITE)                                                                                                           \
    X(ADMIN)

enum DBUserPermissions : U8 {
    PERM_READ = 1 << 0,
    PERM_WRITE = 1 << 1,
    PERM_ADMIN = PERM_READ | PERM_WRITE,
};

struct DBUser {
    DBUser(ok::StringView name, ok::StringView password, U8 perm);

    static DBUser admin() {
        return DBUser{"admin"_sv, "admin"_sv, PERM_ADMIN};
    }

    DBUser *next;
    ok::StringView name;
    ok::Array<U8, 32> sha256_password_digest;
    U8 perm;
};
}; // namespace xmdb
