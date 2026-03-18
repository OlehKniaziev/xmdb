#pragma once

#include "ok.hpp"

namespace xmdb {
/**
 * @brief A string with a fixed maximum size, suitable for storage in table records.
 */
struct FixedString {
    static constexpr UZ DATA_SIZE = 63;  ///< Maximum number of characters.
    static constexpr UZ PREFIX_SIZE = 1; ///< Size of the length prefix.

    /**
     * @brief Gets a view of the string.
     * @return An ok::StringView representing the string content.
     */
    ok::StringView view() const {
        return {(const char *) items, count};
    }

    U8 count;             ///< Current number of characters in the string.
    U8 items[DATA_SIZE]; ///< The character data.
};

static_assert(sizeof(FixedString::count) == FixedString::PREFIX_SIZE);
static_assert(sizeof(FixedString) == FixedString::PREFIX_SIZE + FixedString::DATA_SIZE);

/**
 * @brief Creates a FixedString from an ok::StringView.
 * @param sv The string view to convert.
 * @return The resulting FixedString.
 */
FixedString create_fixed_string(ok::StringView sv);

static inline bool operator==(const FixedString &lhs, const ok::StringView &rhs) {
    return lhs.view() == rhs;
}
} // namespace xmdb
