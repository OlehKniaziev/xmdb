#pragma once

#include "constants.hpp"

namespace xmdb {
/**
 * @brief Supported pixel formats for image data.
 */
enum class PixelFormat {
    RGB, ///< Red, Green, Blue pixel components, 1 byte for each one.
};

/**
 * @brief Retrieves the size of a single pixel in bytes for a given format.
 * @param format The pixel format.
 * @return The size in bytes.
 */
U8 format_pixel_size_in_bytes(PixelFormat format);

/**
 * @brief Represents a chunk of image data in memory.
 */
struct ImageChunk {
    U32 x;           ///< X-coordinate of the chunk's top-left corner.
    U32 y;           ///< Y-coordinate of the chunk's top-left corner.
    U32 width;       ///< Width of the chunk in pixels.
    U32 height;      ///< Height of the chunk in pixels.
    ok::Slice<U8> data; ///< The raw pixel data.
    PixelFormat pixel_format; ///< The format of the pixel data.
};

/**
 * @brief Represents a chunk of image data as stored on disk.
 */
struct DiskImageChunk {
    U64 x;           ///< X-coordinate of the chunk.
    U64 y;           ///< Y-coordinate of the chunk.
    U64 width;       ///< Width of the chunk.
    U64 height;      ///< Height of the chunk.
    U8 pixel_data[]; ///< Variable-length array of pixel data.
};

constexpr UZ MAX_DISK_IMAGE_SIZE = GiB(4);
constexpr UZ MAX_DISK_IMAGE_CHUNK_SIZE = MiB(64);
constexpr UZ MAX_DISK_IMAGE_CHUNK_DATA_SIZE = MAX_DISK_IMAGE_CHUNK_SIZE - sizeof(DiskImageChunk);
constexpr UZ MAX_DISK_IMAGE_CHUNKS_COUNT = MAX_DISK_IMAGE_SIZE / MAX_DISK_IMAGE_CHUNK_SIZE;
constexpr UZ MAX_IMAGE_DATA_SIZE = MAX_DISK_IMAGE_CHUNK_DATA_SIZE * MAX_DISK_IMAGE_CHUNKS_COUNT;

static_assert(MAX_DISK_IMAGE_SIZE % MAX_DISK_IMAGE_CHUNK_SIZE == 0);

/**
 * @brief Stores indices of image chunks.
 */
struct ChunkIndices {
    U8 count;                       ///< The number of chunk indices.
    U32 items[MAX_DISK_IMAGE_CHUNKS_COUNT]; ///< The chunk indices.
};
} // namespace xmdb
