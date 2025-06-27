#pragma once

#include "ok.hpp"

namespace xmdb {

template <typename T>
struct ComputedTableStreamData {
    using TableStreamComputation = Optional<T> (*)(void *);

    ComputedTableStreamData(TableStreamComputation computation, void *data) : computation{computation}, data{data} {}

    TableStreamComputation computation;
    void *data;
};

template <typename T>
struct TableStream {
    enum Type {
        COMPUTED,
    };

    static TableStream<T> computed(ComputedTableStreamData<T> data) {
        return TableStream<T> {
            .type = COMPUTED,
            .u = {
                .computed = data,
            },
        };
    }

    Optional<T> next();

    Type type;
    union {
        ComputedTableStreamData<T> computed;
    } u;
};

template <typename T>
Optional<T> TableStream<T>::next() {
    switch (type) {
    case COMPUTED: {
        return u.computed.computation(u.computed.data);
    }
    }

    OK_UNREACHABLE();
}
} // namespace xmdb
