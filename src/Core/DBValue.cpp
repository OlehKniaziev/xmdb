#include "DBValue.hpp"

namespace xmdb {
DBValue DBValue::cmp(ok::Allocator *allocator, DBValue other) {
    OK_ASSERT(type == other.type);

    switch (type) {
    case SQL::TYPE_BOOL: {
        StreamPair<bool> *computation_data = allocator->alloc<StreamPair<bool>>();
        computation_data->lhs = u.boolean;
        computation_data->rhs = other.u.boolean;

        auto computation = [](void *data) -> Optional<S64> {
            StreamPair<bool> *stream_pair = static_cast<StreamPair<bool> *>(data);
            Optional<bool> lhs = stream_pair->lhs.next();
            Optional<bool> rhs = stream_pair->rhs.next();

            if (!lhs.has_value()) {
                if (!rhs.has_value()) {
                    return {};
                }

                return 1;
            }

            return (S64)lhs.value - (S64)rhs.value;
        };

        auto reset = [](void *data) -> void {
            StreamPair<bool> *stream_pair = static_cast<StreamPair<bool> *>(data);
            stream_pair->lhs.reset();
            stream_pair->rhs.reset();
        };

        TableStream<S64> result = TableStream<S64>::computed(computation, reset, computation_data);
        return DBValue::integer(result);
    }
    default: OK_TODO();
    }
}

void DBValue::reset() {
    switch (type) {
    case SQL::TYPE_INT: {
        u.integer.reset();
        break;
    }
    case SQL::TYPE_STRING: {
        u.string.reset();
        break;
    }
    case SQL::TYPE_BOOL: {
        u.boolean.reset();
        break;
    }
    case SQL::TYPE_NULL: {
        u.null.reset();
        break;
    }
    case SQL::TYPE_FLOAT:
    case SQL::TYPE_DOUBLE:
    case SQL::TYPE_IMAGE: OK_TODO();

    case SQL::TYPE_TABLE:
    case SQL::TYPE_MAX: OK_UNREACHABLE();
    }
}
} // namespace xmdb
