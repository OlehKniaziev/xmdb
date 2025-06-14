#include "Core/BTree.hpp"

#include <gtest/gtest.h>

using namespace ok::literals;

struct BTreeTest : public testing::Test {
    void SetUp() override {
        ok::Optional<ok::File::OpenError> err = ok::create_temp_file(&db_file);
        OK_ASSERT(!err);
    }

    void TearDown() override {
        db_file.remove();
    }

    ok::File db_file;
};

TEST_F(BTreeTest, constructor) {
    ok::ArenaAllocator arena{};

    xmdb::BTree tree{db_file, &arena};
    ASSERT_EQ(tree.header.node_count, 1);

    ASSERT_NE(tree.root_node, nullptr);
    ASSERT_EQ(tree.root_node->count(), 0);
}

TEST_F(BTreeTest, insert) {

}
