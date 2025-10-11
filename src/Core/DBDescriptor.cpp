#include "DBDescriptor.hpp"

namespace xmdb {
DBDescriptor *DBDescriptor::alloc(ok::Allocator *allocator, ok::StringView name) {
    DBDescriptor *descriptor = allocator->alloc<DBDescriptor>();
    descriptor->name = name;
    descriptor->next = nullptr;
    descriptor->users = nullptr;
    descriptor->tables = ok::List<DBTable *>::alloc(allocator);
    return descriptor;
}
} // namespace xmdb
