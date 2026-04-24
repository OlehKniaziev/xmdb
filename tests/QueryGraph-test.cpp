#include <Core/QueryGraph.hpp>
#include <gtest/gtest.h>

using namespace xmdb;

namespace
{
ok::List<QueryGraph::Node *> graph_nodes(ok::Allocator *allocator,
                                         QueryGraph &graph)
{
    auto list = ok::List<QueryGraph::Node *>::alloc(allocator);

    ok::Optional<QueryGraph::Node *> current_node = graph.root_node();
    while (current_node)
    {
        list.push(current_node.get());
        current_node = current_node.get()->next();
    }

    return list;
}
} // namespace

TEST(QueryGraph, constructor)
{
    ok::ArenaAllocator arena{};
    QueryGraph g{&arena};

    auto nodes = graph_nodes(&arena, g);
    ASSERT_EQ(0, nodes.count);
}
