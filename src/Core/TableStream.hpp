#pragma once

#include "ok.hpp"

namespace xmdb {

template <typename T>
struct ComputedTableStream {
    using Computation = Optional<T> (*)(void *);

    ComputedTableStream(Computation computation, void *data) : computation{computation}, data{data} {}

    Computation computation;
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
        CONSTANT,
        ONCE,
    };

    static TableStream<T> computed(ComputedTableStream<T>::Computation computation, void *data) {
        ComputedTableStream<T> computed_stream{computation, data};
        return TableStream<T> {
            .type = COMPUTED,
            .u = {
                .computed = computed_stream,
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

    static TableStream<T> constant(const T &value) {
        return TableStream<T> {
            .type = CONSTANT,
            .u = {
                .constant = value,
            },
        };
    }

    static TableStream<T> once(const T &value) {
        return TableStream<T> {
            .type = ONCE,
            .u = {
                .once = value,
            },
        };
    }

    static TableStream<T> concat(ok::Allocator *allocator, TableStream<T> &lhs, TableStream<T> &rhs) {
        ok::Pair<TableStream<T>, TableStream<T>> *streams = allocator->alloc<ok::Pair<TableStream<T>, TableStream<T>>>();
        streams->a = lhs;
        streams->b = rhs;

        return TableStream<T>::computed([](void *streams_ptr) {
            ok::Pair<TableStream<T>, TableStream<T>> *streams = static_cast<ok::Pair<TableStream<T>, TableStream<T>> *>(streams_ptr);

            Optional<T> lhs_value = streams->a.next();
            if (lhs_value) return lhs_value;
            return streams->b.next();
        }, streams);
    }

    Optional<T> next();

    Type type;
    union {
        ComputedTableStream<T> computed;
        InMemoryTableStream<T> in_memory;
        T constant;
        Optional<T> once;
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
            u.in_memory.values.items += 1;
            u.in_memory.values.count -= 1;
            return item;
        }

        return {};
    }
    case CONSTANT: {
        return u.constant;
    }
    case ONCE: {
        if (u.once.has_value()) {
            T value = u.once.value;
            u.once = {};
            return value;
        }

        return {};
    }
    }

    OK_UNREACHABLE();
}
} // namespace xmdb
