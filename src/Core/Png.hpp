#pragma once

#include "image.hpp"

namespace xmdb {
/**
 * @brief Header of a Png structure.
 */
struct PngHeader {
    U32 width;  ///< Width of the image in pixels.
    U32 height; ///< Height of the image in pixels.
};

/**
 * @brief Represents a PNG image with its header and metadata.
 */
struct Png {
    PngHeader header;   ///< The PNG header.
    PixelFormat format; ///< The pixel format of the image.
    ChunkIndices indices; ///< Indices of disk chunks comprising full PNG image.
};
} // namespace xmdb
