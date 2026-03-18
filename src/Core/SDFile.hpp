#pragma once

#include "ok.hpp"

namespace xmdb {
/**
 * @brief SD (self-destructing) file is accessible only to the current process and gets deleted on process exit.
 */
struct SDFile {
    int fd; ///< The file descriptor.
};

/**
 * @brief Creates a new SDFile with the given filename.
 * @param filename The name of the file to create.
 * @param out Pointer to store the resulting SDFile.
 * @return true if successful, false otherwise.
 */
bool sd_file_create(ok::StringView filename, SDFile *out);
} // namespace xmdb
