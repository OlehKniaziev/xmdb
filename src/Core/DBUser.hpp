#pragma once

#include "hash.hpp"
#include "ok.hpp"

using namespace ok::literals;

namespace xmdb {

#define XMDB_ENUM_USER_PERMISSIONS                                                                                     \
    X(READ, 1 << 0)                                                                                                    \
    X(WRITE, 1 << 1)                                                                                                   \
    X(ADMIN, (1 << 2) | PERM_READ | PERM_WRITE)

#define X(name, value) PERM_##name = value,

/**
 * @brief Permissions available for a database user.
 */
enum DBUserPermissions : U8 { XMDB_ENUM_USER_PERMISSIONS };

#undef X

/**
 * @brief Represents a database user with specific permissions.
 */
struct DBUser {
    /**
     * @brief Constructs a new DBUser.
     * @param name The user's name.
     * @param password The user's password.
     * @param perm The user's permissions.
     */
    DBUser(ok::StringView name, ok::StringView password, U8 perm);

    /**
     * @brief Creates a default admin user.
     * @return A default admin user.
     */
    static DBUser admin() {
        return DBUser{"admin"_sv, "admin"_sv, PERM_ADMIN};
    }

    DBUser *next;                       ///< Pointer to the next user in a list.
    ok::StringView name;                ///< The user's name.
    SHA256Digest sha256_password_digest; ///< The SHA-256 digest of the user's password.
    U8 perm;                            ///< The user's permissions bitfield.
};
}; // namespace xmdb
