#pragma once

#include "image.hpp"

namespace xmdb {
struct PngHeader {
    U32 width;
    U32 height;
};

struct Png {
    PngHeader header;
    PixelFormat format;
    ChunkIndices indices;
};
} // namespace xmdb
