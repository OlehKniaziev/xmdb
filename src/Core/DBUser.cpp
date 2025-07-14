#include "DBUser.hpp"

namespace xmdb {
DBUser::DBUser(ok::StringView name, ok::StringView password, U8 perm) : name{name}, sha256_password_digest{}, perm{perm} {
    // FIXME(oleh): Actually calculate sha256 hash of the password instead of storing it directly.
    memcpy(sha256_password_digest.items, password.data, password.count);
}
} // namespace xmdb
