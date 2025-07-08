#include "DBDescriptor.hpp"

namespace xmdb {
DBDescriptor *DBDescriptor::alloc(ok::Allocator *allocator, ok::StringView name) {
    DBDescriptor *descriptor = allocator->alloc<DBDescriptor>();
    descriptor->name = name;
    descriptor->next = nullptr;
    descriptor->users = ok::List<DBUser>::alloc(allocator);
    descriptor->tables = ok::List<DBTable *>::alloc(allocator);
    return descriptor;
}
} // namespace xmdb
