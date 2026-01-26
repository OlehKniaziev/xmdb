#pragma once

#include "ok.hpp"

namespace xmdb {
struct SDFile {
    int fd;
};

bool sd_file_create(ok::StringView filename, SDFile *out);
} // namespace xmdb
