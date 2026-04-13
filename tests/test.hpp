#pragma once

#include <Core/ok.hpp>
#include <Core/util.hpp>
#include <base64.h>

namespace xmdb::tests
{
static inline ok::StringView load_test_file(const char *name)
{
    static MallocAllocator alloc{};

    auto this_file = ok::StringView{__FILE__};

    for (SZ i = static_cast<SZ>(this_file.count) - 1; i >= 0; --i)
    {
        if (this_file[i] == '/')
        {
            break;
        }
        else
        {
            this_file.count--;
        }
    }

    char *file_path = new char[this_file.count + strlen(name) + 1];
    sprintf(file_path, OK_SV_FMT "test-files/%s", OK_SV_ARG(this_file), name);

    ok::File f{};
    auto open_err = ok::File::open(&f, file_path);
    if (open_err)
    {
        OK_PANIC_FMT("Failed to open file '%s': %s", file_path,
                     ok::File::error_string(&alloc, open_err.get()).cstr());
    }

    ok::List<U8> contents{};
    auto read_err = f.read_full(&alloc, &contents);
    if (read_err)
    {
        OK_PANIC_FMT("Failed to read file '%s': %s", file_path,
                     ok::File::error_string(&alloc, read_err.get()).cstr());
    }

    delete[] file_path;

    return ok::StringView{reinterpret_cast<const char *>(contents.items),
                          contents.count};
}

static inline ok::StringView load_test_file_base64(const char *name)
{
    auto contents = load_test_file(name);

    web_string_view web_contents = {
            .Items = (U8 *) contents.data,
            .Count = contents.count,
    };

    UZ output_buffer_count = contents.count * 8 / 6 + 3;
    U8 *output_buffer = new U8[output_buffer_count];

    WebBase64Encode(web_contents, output_buffer, &output_buffer_count);

    return ok::StringView{reinterpret_cast<const char *>(output_buffer),
                          output_buffer_count};
}

static inline ok::StringView load_test_file_hex(const char *name)
{
    static MallocAllocator alloc{};
    auto contents = load_test_file(name);

    return to_hex_string(&alloc, contents.slice().cast<U8>()).view();
}
} // namespace xmdb::tests
