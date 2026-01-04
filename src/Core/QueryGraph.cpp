#include "QueryGraph.hpp"
#include "log.hpp"
#include "new.hpp"

namespace xmdb {
void QueryGraph::reset() {
    XMDB_FIXME("this should free the nodes in case we don't use an arena allocator");
    m_root_node = nullptr;
    m_last_node = nullptr;
}
}
