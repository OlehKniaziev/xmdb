#pragma once

#include "ok.hpp"

inline void *operator new(UZ n, ok::Allocator *allocator) noexcept {
    return allocator->raw_alloc(n);
}

inline void operator delete(void *ptr, ok::Allocator *allocator, UZ size) noexcept {
    allocator->raw_dealloc(ptr, size);
}
