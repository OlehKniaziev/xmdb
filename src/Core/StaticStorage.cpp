#include "StaticStorage.hpp"
#include "Result.hpp"
#include "util.hpp"
#include "video.hpp"

#include <SQL/ir.hpp>

namespace xmdb
{
#define FAIL(loc, msg)                                                         \
    do                                                                         \
    {                                                                          \
        return ErrorWithSourceLocation{                                        \
                .message = ok::String::alloc(allocator, (msg)),                \
                .location = (loc),                                             \
        };                                                                     \
    }                                                                          \
    while (false)

#define FAIL_FMT(loc, msg, ...)                                                \
    do                                                                         \
    {                                                                          \
        return ErrorWithSourceLocation{                                        \
                .message = ok::String::format(allocator, (msg), __VA_ARGS__),  \
                .location = (loc),                                             \
        };                                                                     \
    }                                                                          \
    while (false)

#define XMDB_DECLARE_BUILTIN_FUNCTION(name, ret, ...)                          \
    static Result<ret, ErrorWithSourceLocation> builtin_##name(                \
            SourceLocation location, ok::Allocator *allocator, __VA_ARGS__)

XMDB_DECLARE_BUILTIN_FUNCTION(RGB, ImageChunk, U32 width, U32 height,
                              ok::StringView hex_data)
{
    U8 bytes_per_pixel = format_pixel_size_in_bytes(PixelFormat::RGB);
    U64 expected_hex_data_count = width * height * bytes_per_pixel * 2;

    if (hex_data.count != expected_hex_data_count)
    {
        FAIL_FMT(location,
                 "the provided hex data has invalid length (wanted %lu, got "
                 "%zu)",
                 expected_hex_data_count, hex_data.count);
    }

    ok::Optional<ok::Slice<U8>> data_opt = from_hex_string(allocator, hex_data);
    if (!data_opt)
    {
        FAIL(location, "failed to decode RGB data as a valid hex string");
    }

    return ImageChunk{
            .x = 0,
            .y = 0,
            .width = width,
            .height = height,
            .data = data_opt.get(),
            .pixel_format = PixelFormat::RGB,
    };
}

XMDB_DECLARE_BUILTIN_FUNCTION(MEDIA, MediaSource, ok::StringView hex_bytes)
{
    ok::Optional<ok::Slice<U8>> data_opt =
            from_hex_string(allocator, hex_bytes);
    if (!data_opt)
    {
        FAIL(location, "failed to decode media source as a valid hex string");
    }

    return MediaSource::from_slice(data_opt.get());
}

StaticStorage *make_or_get_static_storage()
{
    static bool init = false;
    static StaticStorage storage{};
    static MallocAllocator malloc_allocator{};

    if (!init)
    {
        storage.builtin_functions =
                ok::Table<ok::StringView, void *>::alloc(&malloc_allocator);

#define X(fn_name, ret, ...)                                                   \
    do                                                                         \
    {                                                                          \
        XMDB_BUILTIN_FUNCTION_SIG_NAME(fn_ptr, ret, __VA_ARGS__) =             \
                builtin_##fn_name;                                             \
        storage.builtin_functions.put(ok::StringView{#fn_name},                \
                                      (void *) fn_ptr);                        \
    }                                                                          \
    while (false);
        XMDB_ENUM_BUILTIN_FUNCTIONS
#undef X

        init = true;
    }

    return &storage;
}
} // namespace xmdb
