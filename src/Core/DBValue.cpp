#include "DBValue.hpp"

namespace xmdb {
DBValue DBValue::cmp(QueryGraph &qg, DBValue other) {
    OK_ASSERT(m_type == other.m_type);

    QueryGraph::CompareNode *cmp_node = qg.compare(m_node_repr, other.m_node_repr);
    return DBValue{m_type, cmp_node};
}
} // namespace xmdb
