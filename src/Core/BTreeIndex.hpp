#pragma once

#include "ok.hpp"

namespace xmdb {
/**
 * @brief A B-Tree index for efficient key-value lookups and storage.
 */
struct BTreeIndex {
    /**
     * @brief Creates a new BTreeIndex from a state file.
     * @param allocator The allocator to use.
     * @param state_filename The path to the state file.
     * @param index Pointer to store the resulting index.
     * @return An optional open error if the operation failed.
     */
    static ok::Optional<ok::File::OpenError> create(ok::Allocator *allocator, ok::StringView state_filename,
                                                    BTreeIndex *index);

    /**
     * @brief Creates a BTreeIndex from an already open state file.
     * @param allocator The allocator to use.
     * @param state_file The open state file.
     * @return The resulting BTreeIndex.
     */
    static BTreeIndex create(ok::Allocator *allocator, ok::File state_file);

    /**
     * @brief Inserts a key into the index.
     * @param key The key to insert.
     */
    void insert(U64 key);

    /**
     * @brief Checks if the index contains a specific key.
     * @param key The key to look for.
     * @return true if found, false otherwise.
     */
    bool contains(U64 key);

    /**
     * @brief Removes a key from the index.
     * @param key The key to remove.
     * @return true if successful, false otherwise.
     */
    bool remove(U64 key);

    /**
     * @brief Checks if the index was just first_constructed.
     * @return true if it was first_constructed, false otherwise.
     */
    bool first_constructed();

    /**
     * @brief Gets the order of the B-Tree.
     * @return The order.
     */
    UZ order();

    /**
     * @brief Gets the height of the B-Tree.
     * @return The height.
     */
    U64 height();

    /**
     * @brief Gets the total number of nodes in the B-Tree.
     * @return The node count.
     */
    U64 node_count();

    /**
     * @brief Gets the total number of key-value pairs in the index.
     * @return The KV count.
     */
    U64 total_kv_count();

    /**
     * @brief Resets the index, clearing all data.
     * @return true if successful, false otherwise.
     */
    bool reset();

    void *pImpl; ///< Pointer to the internal implementation.
};
}; // namespace xmdb
