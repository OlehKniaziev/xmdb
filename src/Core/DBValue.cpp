#include "DBValue.hpp"

namespace xmdb {
template <typename T>
struct StreamPair {
    TableStream<T> lhs;
    TableStream<T> rhs;
};

DBValue DBValue::cmp(ok::Allocator *allocator, DBValue other) {
    OK_ASSERT(type == other.type);

    switch (type) {
    case SQL::TYPE_BOOL: {
        StreamPair<bool> *computation_data = allocator->alloc<StreamPair<bool>>();
        computation_data->lhs = u.b;
        computation_data->rhs = other.u.b;

        ComputedTableStreamData<S64> computation{[](void *data) -> Optional<S64> {
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
        }, computation_data};

        TableStream<S64> result = TableStream<S64>::computed(computation);
        return DBValue::integer(result);
    }
    default: OK_TODO();
    }
}
} // namespace xmdb
