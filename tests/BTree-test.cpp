#include <Core/BTree.hpp>
#include <gtest/gtest.h>

using namespace xmdb;

static BTreeIndex default_index() {
    ok::ArenaAllocator arena{};
    ok::File state_file{};

    OK_VERIFY(!ok::create_temp_file(&state_file).has_value());

    return BTreeIndex::create(&arena, state_file);
}

// NOTE(oleh): Just to ensure that the constructor will not crash.
TEST(BTreeIndexTest, create_with_temp_state_file) {
    ok::ArenaAllocator arena{};
    ok::File state_file{};

    OK_VERIFY(!ok::create_temp_file(&state_file).has_value());

    auto tree = BTreeIndex::create(&arena, state_file);
    OK_UNUSED(tree);
}

TEST(BTreeIndexTest, insert) {
    BTreeIndex index = default_index();

    index.insert(0);
    index.insert(1);
    index.insert(2);

    ASSERT_TRUE(index.contains(0));
    ASSERT_TRUE(index.contains(1));
    ASSERT_TRUE(index.contains(2));
}
