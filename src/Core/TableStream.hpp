#pragma once

#include "ok.hpp"

namespace xmdb {

template <typename T>
struct ComputedTableStream {
    using TableStreamComputation = Optional<T> (*)(void *);

    ComputedTableStream(TableStreamComputation computation, void *data) : computation{computation}, data{data} {}

    TableStreamComputation computation;
    void *data;
};

template <typename T>
struct InMemoryTableStream {
    InMemoryTableStream(ok::Slice<T> values) : values{values} {}
    Slice<T> values;
};

template <typename T>
struct TableStream {
    enum Type {
        COMPUTED,
        IN_MEMORY,
    };

    static TableStream<T> computed(ComputedTableStream<T> computed) {
        return TableStream<T> {
            .type = COMPUTED,
            .u = {
                .computed = computed,
            },
        };
    }

    static TableStream<T> in_memory(InMemoryTableStream<T> in_memory) {
        return TableStream<T> {
            .type = IN_MEMORY,
            .u = {
                .in_memory = in_memory,
            },
        };
    }

    static TableStream<T> empty() {
        ok::Slice<T> empty_slice{};
        empty_slice.items = nullptr;
        empty_slice.count = 0;
        return TableStream<T>::in_memory(empty_slice);
    }

    Optional<T> next();

    Type type;
    union {
        ComputedTableStream<T> computed;
        InMemoryTableStream<T> in_memory;
    } u;
};

template <typename T>
Optional<T> TableStream<T>::next() {
    switch (type) {
    case COMPUTED: {
        return u.computed.computation(u.computed.data);
    }
    case IN_MEMORY: {
        if (u.in_memory.values.count > 0) {
            const T &item = u.in_memory.values[0];
            u.in_memory.values.count -= 1;
            return item;
        }

        return {};
    }
    }

    OK_UNREACHABLE();
}
} // namespace xmdb
