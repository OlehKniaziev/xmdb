#pragma once

namespace xmdb {
enum class PngFormat {
    RGB,
    RGBA,
};

struct PngHeader {
    U32 width;
    U32 height;
    FixedString filepath;
};

struct Png {
    PngHeader header;
    PngFormat format;
    U8 *pixels;
};
} // namespace xmdb
