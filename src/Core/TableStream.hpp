#pragma once

#include "ok.hpp"
#include "PagedSeekList.hpp"

namespace xmdb {

template <typename T>
struct ComputedTableStream {
    using Computation = ok::Optional<T> (*)(void *);
    using Reset = void (*)(void *);

    ComputedTableStream(Computation computation, Reset reset, void *data) : computation{computation}, reset{reset}, data{data} {}

    Computation computation;
    Reset reset;
    void *data;
};

template <typename T>
struct InMemoryTableStream {
    explicit InMemoryTableStream(ok::Slice<T> values) : offset{0}, values{values} {}
    UZ offset;
    ok::Slice<T> values;
};

template <typename T, UZ A = alignof(T)>
struct DiskTableStream {
    PagedSeekList *page_list;
};

template <typename T>
struct TableStream {
    enum Type {
        COMPUTED,
        IN_MEMORY,
        CONSTANT,
        ONCE,
    };

    static TableStream<T> computed(ComputedTableStream<T>::Computation computation,
                                   ComputedTableStream<T>::Reset reset,
                                   void *data) {
        ComputedTableStream<T> computed_stream{computation, reset, data};
        return TableStream<T> {
            .type = COMPUTED,
            .u = {
                .computed = computed_stream,
            },
        };
    }

    static TableStream<T> in_memory(ok::Slice<T> values) {
        return TableStream<T> {
            .type = IN_MEMORY,
            .u = {
                .in_memory = InMemoryTableStream<T> {values},
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
                .once = {
                    .value = value,
                    .has_value = true,
                },
            },
        };
    }

    static TableStream<T> concat(ok::Allocator *allocator, TableStream<T> &lhs, TableStream<T> &rhs) {
        ok::Pair<TableStream<T>, TableStream<T>> *streams = allocator->alloc<ok::Pair<TableStream<T>, TableStream<T>>>();
        streams->a = lhs;
        streams->b = rhs;

        auto computation = [](void *streams_ptr) -> ok::Optional<T> {
            ok::Pair<TableStream<T>, TableStream<T>> *streams = static_cast<ok::Pair<TableStream<T>, TableStream<T>> *>(streams_ptr);

            ok::Optional<T> lhs_value = streams->a.next();
            if (lhs_value) return lhs_value;
            return streams->b.next();
        };
        auto reset = [](void *streams_ptr) -> void {
            ok::Pair<TableStream<T>, TableStream<T>> *streams = static_cast<ok::Pair<TableStream<T>, TableStream<T>> *>(streams_ptr);
            streams->a.reset();
            streams->b.reset();
        };

        return TableStream<T>::computed(computation,
                                        reset,
                                        streams);
    }

    struct Once {
        T value;
        bool has_value;
    };

    Optional<T> next();
    void reset();

    Type type;
    union {
        ComputedTableStream<T> computed;
        InMemoryTableStream<T> in_memory;
        DiskTableStream<T> on_disk;
        T constant;
        Once once;
    } u;
};

template <typename T>
struct StreamPair {
    TableStream<T> lhs;
    TableStream<T> rhs;
};

template <typename T>
Optional<T> TableStream<T>::next() {
    switch (type) {
    case COMPUTED: {
        return u.computed.computation(u.computed.data);
    }
    case IN_MEMORY: {
        if (u.in_memory.offset < u.in_memory.values.count) {
            const T &item = u.in_memory.values[u.in_memory.offset];
            u.in_memory.offset += 1;
            return item;
        }

        return {};
    }
    case CONSTANT: {
        return u.constant;
    }
    case ONCE: {
        if (u.once.has_value) {
            T value = u.once.value;
            u.once.has_value = false;
            return value;
        }

        return {};
    }
    }

    OK_UNREACHABLE();
}

template <typename T>
void TableStream<T>::reset() {
    switch (type) {
    case COMPUTED: {
        u.computed.reset(u.computed.data);
        break;
    }
    case IN_MEMORY: {
        u.in_memory.offset = 0;
        break;
    }
    case ONCE: {
        u.once.has_value = true;
        break;
    }
    case CONSTANT: break;
    }
}
} // namespace xmdb
