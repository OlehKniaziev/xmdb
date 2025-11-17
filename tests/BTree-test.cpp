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

    ASSERT_FALSE(ok::create_temp_file(&state_file).has_value());

    auto tree = BTreeIndex::create(&arena, state_file);
    OK_UNUSED(tree);
}

TEST(BTreeIndexTest, create_then_reload) {
    ok::ArenaAllocator arena{};
    ok::File state_file{};

    EXPECT_FALSE(ok::create_temp_file(&state_file).has_value());

    auto tree = BTreeIndex::create(&arena, state_file);

    EXPECT_TRUE(tree.first_constructed());

    constexpr auto value = 55;

    tree.insert(value);

    tree = BTreeIndex::create(&arena, state_file);

    EXPECT_FALSE(tree.first_constructed());

    EXPECT_TRUE(tree.contains(value));

    ASSERT_FALSE(state_file.remove());
}

TEST(BTreeIndexTest, contains_after_insert) {
    BTreeIndex index = default_index();

    index.insert(0);
    index.insert(1);
    index.insert(2);

    ASSERT_TRUE(index.contains(0));
    ASSERT_TRUE(index.contains(1));
    ASSERT_TRUE(index.contains(2));
}
