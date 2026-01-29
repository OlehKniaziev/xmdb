#pragma once

#include "constants.hpp"

namespace xmdb {
enum class PixelFormat {
    RGB,
};

U8 format_pixel_size_in_bytes(PixelFormat);

struct ImageChunk {
    U32 x;
    U32 y;
    U32 width;
    U32 height;
    ok::Slice<U8> data;
    PixelFormat format;
};

struct DiskImageChunk {
    U64 x;
    U64 y;
    U64 width;
    U64 height;
    U8 pixel_data[];
};

constexpr UZ MAX_DISK_IMAGE_SIZE = GiB(4);
constexpr UZ MAX_DISK_IMAGE_CHUNK_SIZE = MiB(64);
constexpr UZ MAX_DISK_IMAGE_CHUNK_DATA_SIZE = MAX_DISK_IMAGE_CHUNK_SIZE - sizeof(DiskImageChunk);
constexpr UZ MAX_DISK_IMAGE_CHUNKS_COUNT = MAX_DISK_IMAGE_SIZE / MAX_DISK_IMAGE_CHUNK_SIZE;
constexpr UZ MAX_IMAGE_DATA_SIZE = MAX_DISK_IMAGE_CHUNK_DATA_SIZE * MAX_DISK_IMAGE_CHUNKS_COUNT;

static_assert(MAX_DISK_IMAGE_SIZE % MAX_DISK_IMAGE_CHUNK_SIZE == 0);

struct ChunkIndices {
    U8 count;
    U32 items[MAX_DISK_IMAGE_CHUNKS_COUNT];
};
} // namespace xmdb
