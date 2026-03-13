#include "image.hpp"

namespace xmdb {
U8 format_pixel_size_in_bytes(PixelFormat format) {
    switch (format) {
    case PixelFormat::RGB: return 3;
    }

    OK_UNREACHABLE();
}
} // namespace xmdb
