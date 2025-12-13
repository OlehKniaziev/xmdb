#pragma once

#include "ok.hpp"

struct PagedSeekList {
    static PagedSeekList *alloc(ok::Allocator *allocator, ok::File file, UZ page_size) {
        auto *list = allocator->alloc<PagedSeekList>();
        list->current_page = nullptr;
        list->page_offset = 0;
        list->file_offset = 0;
        list->page_size = page_size;
        list->backing_file = file;
        return list;
    }

    inline void advance(UZ n) {
        file_offset += n;
    }

    ok::Optional<ok::File::ReadError> get_page(void **result) {
        UZ wanted_page_offset = ok::align_down(file_offset, page_size);

        if (current_page == nullptr) {
            auto err = load_page(wanted_page_offset, result);
            if (err) return err;

            page_offset = wanted_page_offset;
            current_page = *result;
            return {};
        }

        if (wanted_page_offset == page_offset) {
            *result = current_page;
            return {};
        }

        OK_TODO();
    }

    ok::Optional<ok::File::ReadError> load_page(UZ offset, void **result) {
        OK_TODO();
    }

    void *current_page;
    UZ page_offset;
    UZ file_offset;
    UZ page_size;
    ok::File backing_file;
};
