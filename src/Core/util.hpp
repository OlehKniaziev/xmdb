#pragma once

#include "SourceLocation.hpp"
#include "meta.hpp"
#include "ok.hpp"

#if !defined(OK_NO_STDLIB)
#include <common.h>
#endif // !OK_NO_STDLIB

#define TRY(x)                                                                 \
    do {                                                                       \
        if (!(x)) return {};                                                   \
    } while (0)

namespace xmdb {
/**
 * @brief Terminates the program with a formatted error message.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
[[noreturn]]
void dief(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);

/**
 * @brief Represents an error message paired with its source location.
 */
struct ErrorWithSourceLocation {
    ok::String message; ///< The error message.
    SourceLocation location; ///< The location in the source code where the
                             ///< error occurred.
};

/**
 * @brief Converts a slice of bytes to a hexadecimal string.
 * @param allocator The allocator to use for the string.
 * @param bytes The bytes to convert.
 * @return The hexadecimal string.
 */
ok::String to_hex_string(ok::Allocator *allocator, ok::Slice<U8> bytes);

/**
 * @brief Converts a hexadecimal string back to a slice of bytes.
 * @param allocator The allocator to use for the resulting slice.
 * @param sv The hexadecimal string.
 * @return An optional slice of bytes if conversion succeeded.
 */
ok::Optional<ok::Slice<U8>> from_hex_string(ok::Allocator *allocator,
                                            ok::StringView sv);

template<typename T>
RemoveRef<T> &&move(T &&arg) {
    return static_cast<RemoveRef<T> &&>(arg);
}

template<typename T>
T &&forward(RemoveRef<T> &val) {
    return static_cast<T&&>(val);
}

template<typename T>
T &&forward(RemoveRef<T> &&val) {
    static_assert(!IsLValueRef<T>::Value);
    return static_cast<T&&>(val);
}

#if !defined(OK_NO_STDLIB)
/**
 * @brief An allocator that uses a web_arena for memory management.
 */
struct WebArenaAllocator : public ok::Allocator {
    /**
     * @brief Constructs a new WebArenaAllocator.
     * @param impl Pointer to the web_arena implementation.
     */
    explicit WebArenaAllocator(web_arena *impl) : impl{impl} {
    }

    void *raw_alloc(UZ size) override {
        return WebArenaPush(impl, size);
    }

    void raw_dealloc(void *ptr, UZ size) override {
        (void) ptr;
        (void) size;
    }

    void *raw_resize(void *ptr, UZ old_size, UZ new_size) override {
        return WebArenaRealloc(impl, ptr, old_size, new_size);
    }

    web_arena *impl; ///< The underlying web_arena.
};

/**
 * @brief An allocator that uses standard malloc/free for memory management.
 */
struct MallocAllocator : public ok::Allocator {
    void *raw_alloc(UZ size) override {
        return calloc(1, size);
    }

    void raw_dealloc(void *ptr, UZ size) override {
        (void) size;
        free(ptr);
    }

    void *raw_resize(void *ptr, UZ old_size, UZ new_size) override {
        (void) old_size;
        return realloc(ptr, new_size);
    }
};

#endif // !OK_NO_STDLIB
} // namespace xmdb
