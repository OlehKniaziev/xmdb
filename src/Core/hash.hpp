#pragma once

#include "ok.hpp"

namespace xmdb {
/**
 * @brief Represents a SHA-256 hash digest.
 */
struct SHA256Digest {
    static constexpr UZ SIZE = 32; ///< Size of the digest in bytes.

    inline bool operator==(const SHA256Digest &other) const {
        for (UZ i = 0; i < SIZE; ++i) {
            if (bytes[i] != other.bytes[i]) return false;
        }

        return true;
    }

    ok::Array<U8, SIZE> bytes; ///< The raw bytes of the digest.
};

/**
 * @brief Computes the SHA-256 digest of a string view.
 * @param sv The string view to hash.
 * @return The resulting SHA256Digest.
 */
SHA256Digest sha256_digest(ok::StringView sv);
}; // namespace xmdb
