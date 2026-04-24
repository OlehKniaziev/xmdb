#pragma once

#include "ok.hpp"

/**
 * @brief Provides paged access to a file, allowing efficient seeking and
 * reading.
 */
struct PagedSeekList
{
    /**
     * @brief Constructs a new PagedSeekList.
     * @param allocator The allocator to use for page buffers.
     * @param file The backing file to read from.
     * @param page_size The size of each page in bytes.
     */
    PagedSeekList(ok::Allocator *allocator, ok::File file, UZ page_size);

    /**
     * @brief Advances the current file offset.
     * @param n The number of bytes to advance.
     */
    inline void advance(UZ n)
    {
        file_offset += n;
    }

    /**
     * @brief Retrieves data at the current file offset.
     * @param out_data Pointer to store the address of the data.
     * @param requested The number of bytes requested.
     * @return An optional read error if the operation failed.
     */
    ok::Optional<ok::File::ReadError> get_data(U8 **out_data, UZ requested);

    /**
     * @brief Loads a page of data from the file.
     * @param offset The file offset to load from.
     * @param result Pointer to store the address of the loaded page.
     * @param bytes_avail Pointer to store the number of bytes available in the
     * page.
     * @return An optional read error if the operation failed.
     */
    ok::Optional<ok::File::ReadError> load_page(UZ offset, U8 **result,
                                                UZ *bytes_avail);

    /**
     * @brief Checks if the current offset is at the end of the file.
     * @return true if at the end, false otherwise.
     */
    bool is_at_end();

    /**
     * @brief Resets the file offset and page state to the beginning.
     */
    void reset();

    ok::Allocator *allocator; ///< The allocator for page buffers.
    U8 *current_page = nullptr; ///< Pointer to the currently loaded page.
    UZ current_page_avail =
            0; ///< Number of bytes available in the current page.
    UZ page_offset = 0; ///< The file offset of the current page.
    UZ file_offset = 0; ///< The current logical offset in the file.
    UZ page_size; ///< The size of each page.
    ok::File backing_file; ///< The backing file.
};
