#pragma once

#include "ok.hpp"

namespace xmdb {
enum DBUserPermissions : U8 {
    PERM_READ,
    PERM_WRITE,
    PERM_ADMIN,
};

struct DBUser {
    DBUser(ok::StringView name, ok::StringView password, U8 perm);

    ok::StringView name;
    ok::Array<U8, 32> sha256_password_digest;
    U8 perm;
};
};
