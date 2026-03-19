#pragma once

#include "ok.hpp"

/**
 * @brief Placement new operator that uses an ok::Allocator.
 * @param n The size to allocate.
 * @param allocator The allocator to use.
 * @return Pointer to the allocated memory.
 */
inline void *operator new(UZ n, ok::Allocator *allocator) noexcept {
    return allocator->raw_alloc(n);
}

/**
 * @brief Placement delete operator matching the placement new with ok::Allocator.
 * @param ptr Pointer to the memory to deallocate.
 * @param allocator The allocator used for allocation.
 * @param size The size of the allocation.
 */
inline void operator delete(void *ptr, ok::Allocator *allocator, UZ size) noexcept {
    allocator->raw_dealloc(ptr, size);
}
