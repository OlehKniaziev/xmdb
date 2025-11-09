#pragma once

#include "hash.hpp"
#include "ok.hpp"

using namespace ok::literals;

namespace xmdb {

#define XMDB_ENUM_USER_PERMISSIONS                                                                                     \
    X(READ, 1 << 0)                                                                                                    \
    X(WRITE, 1 << 1)                                                                                                   \
    X(ADMIN, PERM_READ | PERM_WRITE)

#define X(name, value) PERM_##name = value,

enum DBUserPermissions : U8 { XMDB_ENUM_USER_PERMISSIONS };

#undef X

struct DBUser {
    DBUser(ok::StringView name, ok::StringView password, U8 perm);

    static DBUser admin() {
        return DBUser{"admin"_sv, "admin"_sv, PERM_ADMIN};
    }

    DBUser *next;
    ok::StringView name;
    SHA256Digest sha256_password_digest;
    U8 perm;
};
}; // namespace xmdb
