#pragma once

#include "ok.hpp"

struct PagedSeekList {
    PagedSeekList(ok::Allocator *allocator, ok::File file, UZ page_size) :
        allocator{allocator}, page_size{page_size}, backing_file{file} {
    }

    inline void advance(UZ n) {
        file_offset += n;
    }

    ok::Optional<ok::File::ReadError> get_data(U8 **out_data, UZ requested) {
        UZ wanted_page_offset = ok::align_down(file_offset, page_size);

        if (current_page == nullptr || wanted_page_offset != page_offset || requested > current_page_avail) {
            UZ avail = 0;
            if (auto err = load_page(wanted_page_offset, out_data, &avail); err) return err;

            if (avail < requested)
                OK_PANIC_FMT("Not enough file data: requested %zu bytes, available %zu", avail, requested);

            page_offset = wanted_page_offset;
            current_page = *out_data;
            current_page_avail = avail;

            return {};
        }

        *out_data = current_page;
        return {};
    }

    ok::Optional<ok::File::ReadError> load_page(UZ offset, U8 **result, UZ *bytes_avail) {
        OK_ASSERT((page_size & 1) == 0);
        OK_ASSERT((offset & (page_size - 1)) == 0);

        if (current_page != nullptr && page_offset == offset) {
            *result = current_page;
            return {};
        }

        auto *page_buffer = allocator->alloc<U8>(page_size);

        backing_file.seek_to(offset);

        if (auto err = backing_file.read(page_buffer, page_size, bytes_avail); err.has_value()) {
            return err;
        }

        *result = page_buffer;
        current_page = page_buffer;
        page_offset = offset;
        file_offset = page_offset;

        return {};
    }

    bool is_at_end() {
        return file_offset >= backing_file.size();
    }

    void reset() {
        if (page_offset == 0) {
            return;
        }

        page_offset = 0;
        file_offset = 0;
        current_page = nullptr;
        current_page_avail = 0;
    }

    ok::Allocator *allocator;
    U8 *current_page = nullptr;
    UZ current_page_avail = 0;
    UZ page_offset = 0;
    UZ file_offset = 0;
    UZ page_size;
    ok::File backing_file;
};
