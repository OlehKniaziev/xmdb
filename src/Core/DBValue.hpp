#pragma once

#include <SQL/type_check.hpp>

#include "QueryGraph.hpp"

namespace xmdb {
class DBValue {
public:
    DBValue() = delete;

    // DBValue(const DBValue &) = delete;
    // DBValue(DBValue &&) = delete;

    DBValue(SQL::Type type, const QueryGraph::Node *node) : m_type{type}, m_node_repr{node} {}

    static DBValue concat(ok::Allocator *, DBValue, DBValue) {
        OK_TODO();
    }

    static DBValue const_bool(QueryGraph &graph, bool b) {
        QueryGraph::ConstantNode *node = graph.const_bool(b);
        return DBValue{SQL::TYPE_BOOL, node};
    }

    static DBValue const_int(QueryGraph &graph, S64 i) {
        QueryGraph::ConstantNode *node = graph.const_int(i);
        return DBValue{SQL::TYPE_INT, node};
    }

    static DBValue const_string(QueryGraph &graph, ok::StringView s) {
        QueryGraph::ConstantNode *node = graph.const_string(s);
        return DBValue{SQL::TYPE_STRING, node};
    }

    static DBValue null() {
        OK_TODO();
    }

    SQL::Type type() const {
        return m_type;
    }

    const QueryGraph::Node *node_repr() const {
        return m_node_repr;
    }

    DBValue cmp(QueryGraph &, DBValue);

private:
    SQL::Type m_type;
    const QueryGraph::Node *m_node_repr;
};
} // namespace xmdb
