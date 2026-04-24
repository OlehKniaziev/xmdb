#include "obj.hpp"

namespace xmdb::server
{
ok::Slice<TableObject> get_db_table_objects(ok::Allocator *allocator,
                                            DBDescriptor *db)
{
    ok::List<TableObject> objs = ok::List<TableObject>::alloc(allocator);

    for (UZ i = 0; i < db->tables.count; ++i)
    {
        DBTable *table = db->tables[i];
        objs.push(TableObject{
                .name = table->name(),
                .column_names = table->columns_names(),
                .column_types = table->columns_types(),
        });
    }

    return objs.slice();
}
} // namespace xmdb::server
