#include "SDFile.hpp"
#include "Logger.hpp"

namespace xmdb {
bool sd_file_create(ok::StringView filename, SDFile *out) {
    bool result = true;
    int fd = -1;

    char *filename_cstr = ok::temp_allocator()->alloc<char>(filename.count + 1);
    memcpy(filename_cstr, filename.data, filename.count);

    fd = open(filename_cstr, O_CREAT|O_EXCL|O_RDWR|O_SYNC, 0666);
    if (fd == -1) {
        result = false;
        goto defer;
    }

    // NOTE(oleh): Doing this will delete the file at process exit time, assuming no other process
    // opens it.
    if (unlink(filename_cstr) == -1) {
        log::error("Failed to unlink a self-destructing file: %s", strerror(errno));
        result = false;
        goto defer;
    }

    out->fd = fd;

defer:

    if (!result && fd != -1) close(fd);
    return result;
}
}
