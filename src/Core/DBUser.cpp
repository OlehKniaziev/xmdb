#include "DBUser.hpp"

namespace xmdb {
DBUser::DBUser(ok::StringView name, ok::StringView password, U8 perm) :
    name{name}, sha256_password_digest{}, perm{perm} {
    sha256_password_digest = sha256_digest(password);
}
} // namespace xmdb
