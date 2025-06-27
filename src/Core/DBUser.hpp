#pragma once

#include "ok.hpp"

namespace xmdb {
enum DBUserPermissions : U8 {
    PERM_READ,
    PERM_WRITE,
};

struct DBUser {
    ok::StringView name;
    U8 password_sha256_digest[32];
    U8 perm;
};
};
