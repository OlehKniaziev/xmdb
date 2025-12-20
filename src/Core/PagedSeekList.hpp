#pragma once

#include "ok.hpp"

struct PagedSeekList {
    static PagedSeekList *alloc(ok::Allocator *allocator, ok::File file, UZ page_size) {
        auto *list = allocator->alloc<PagedSeekList>();
        list->current_page = nullptr;
        list->current_page_avail = 0;
        list->page_offset = 0;
        list->file_offset = 0;
        list->page_size = page_size;
        list->backing_file = file;
        return list;
    }

    inline void advance(UZ n) {
        file_offset += n;
    }

    ok::Optional<ok::File::ReadError> get_page(void **result, UZ *result_avail) {
        UZ wanted_page_offset = ok::align_down(file_offset, page_size);

        if (current_page == nullptr || wanted_page_offset != page_offset) {
            if (auto err = load_page(wanted_page_offset, result, result_avail); err) return err;

            page_offset = wanted_page_offset;
            current_page = *result;
            current_page_avail = *result_avail;

            return {};
        }

        *result = current_page;
        return {};
    }

    ok::Optional<ok::File::ReadError> load_page(UZ offset, void **result, UZ *bytes_avail) {
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

    ok::Allocator *allocator;
    void *current_page;
    UZ current_page_avail;
    UZ page_offset;
    UZ file_offset;
    UZ page_size;
    ok::File backing_file;
};
