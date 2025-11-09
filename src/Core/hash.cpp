#include "hash.hpp"

namespace xmdb {
SHA256Digest sha256_digest(ok::StringView sv) {
    SHA256Digest digest{};

    UZ count = ok::min(sv.count, SHA256Digest::SIZE);
    memcpy(digest.bytes.items, sv.data, count);

    return digest;
}
}; // namespace xmdb
