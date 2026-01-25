#pragma once

#include "ok.hpp"

namespace xmdb {
struct BTreeIndex {
    static ok::Optional<ok::File::OpenError> create(ok::Allocator *allocator, ok::StringView state_filename,
                                                    BTreeIndex *index);
    static BTreeIndex create(ok::Allocator *allocator, ok::File state_file);

    void insert(U64);

    bool contains(U64);

    bool remove(U64);

    bool first_constructed();

    UZ order();

    U64 height();

    U64 node_count();

    bool reset();

    void *pImpl;
};
}; // namespace xmdb
