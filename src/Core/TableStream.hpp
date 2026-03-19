#pragma once

#include "PagedSeekList.hpp"
#include "constants.hpp"
#include "ok.hpp"

namespace xmdb {
class RowDescriptor;

/**
 * @brief Describes a column in a row.
 */
class ColumnDescriptor {
public:
    /**
     * @brief Constructs a new ColumnDescriptor.
     * @param row_desc Reference to the row descriptor this column belongs to.
     * @param index The index of the column.
     */
    explicit ColumnDescriptor(RowDescriptor &row_desc, U64 index);

    /**
     * @brief Gets the column index.
     * @return The index.
     */
    U64 index() const {
        return m_index;
    }

private:
    RowDescriptor &m_row_descriptor;
    U64 m_index;
};

/**
 * @brief Describes a row in a table.
 */
class RowDescriptor {
public:
    /**
     * @brief Gets the collection of column descriptors.
     * @return A slice of column descriptors.
     */
    ok::Slice<ColumnDescriptor> column_descriptors() const {
        return m_column_descriptors;
    }
private:
    ok::Slice<ColumnDescriptor> m_column_descriptors;
};

/**
 * @brief Base class for table storage backends.
 */
class Store {
public:
    Store() = delete;

    template<typename T>
    const T &deref(ColumnDescriptor) = delete;

protected:
    /**
     * @brief Storage backend type.
     */
    enum class Type {
        RAM,  ///< In-memory storage.
        DISK, ///< Disk-based storage.
    };

    /**
     * @brief Constructs a Store with a specific type.
     * @param type The storage type.
     */
    Store(Type type) : m_type{type} {
    }

    Type m_type;
};

/**
 * @brief Disk-based storage backend.
 */
class DiskStore : public Store {
public:
    /**
     * @brief Constructs a new DiskStore.
     */
    DiskStore() : Store(Store::Type::DISK) {
    }

private:
};

} // namespace xmdb
