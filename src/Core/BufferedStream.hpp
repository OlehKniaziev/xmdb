#pragma once

#include "ok.hpp"
#include "Result.hpp"

namespace xmdb {
struct BufferedStream {
    BufferedStream() = delete;

    enum class Type {
        DISK,
    };

    // Num of bytes or an error string.
    Result<UZ, ok::String> pull(ok::Allocator *allocator);

    Type type;
    ok::Slice<U8> buffer;

    union {
        ok::File file;
    } u;
};
} // namespace xmdb
