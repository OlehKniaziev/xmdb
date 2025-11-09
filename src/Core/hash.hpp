#pragma once

#include "ok.hpp"

namespace xmdb {
struct SHA256Digest {
    static constexpr UZ SIZE = 32;

    inline bool operator==(const SHA256Digest &other) const {
        for (UZ i = 0; i < SIZE; ++i) {
            if (bytes[i] != other.bytes[i]) return false;
        }

        return true;
    }

    ok::Array<U8, SIZE> bytes;
};

SHA256Digest sha256_digest(ok::StringView sv);
}; // namespace xmdb
