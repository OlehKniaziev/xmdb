#pragma once

#include "PagedSeekList.hpp"
#include "constants.hpp"
#include "ok.hpp"

namespace xmdb {
class RowDescriptor;

class ColumnDescriptor {
public:
    explicit ColumnDescriptor(RowDescriptor &row_desc, U64 index);

    U64 index() const {
        return m_index;
    }

private:
    RowDescriptor &m_row_descriptor;
    U64 m_index;
};

class RowDescriptor {
public:
    ok::Slice<ColumnDescriptor> column_descriptors() const {
        return m_column_descriptors;
    }
private:
    ok::Slice<ColumnDescriptor> m_column_descriptors;
};

class Store {
public:
    Store() = delete;

    template<typename T>
    const T &deref(ColumnDescriptor) = delete;

protected:
    enum class Type {
        RAM,
        DISK,
    };

    Store(Type type) : m_type{type} {
    }

    Type m_type;
};

class DiskStore : public Store {
public:
    DiskStore() : Store(Store::Type::DISK) {
    }

private:
};

} // namespace xmdb
