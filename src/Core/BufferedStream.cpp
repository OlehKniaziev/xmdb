#include "BufferedStream.hpp"

namespace xmdb
{
Result<UZ, ok::String> BufferedStream::pull(ok::Allocator *allocator)
{
    switch (type)
    {
    case Type::DISK:
    {
        ok::File *file = &u.file;
        UZ n_read = 0;
        ok::Optional<ok::File::IOError> read_err =
                file->read(buffer.items, buffer.count, &n_read);
        if (read_err)
        {
            auto error_string =
                    ok::File::error_string(allocator, read_err.get());
            return ok::String::format(
                    allocator, "Failed to read from the disk stream file: %s",
                    error_string.cstr());
        }

        return n_read;
    }
    }

    OK_UNREACHABLE();
}
} // namespace xmdb
