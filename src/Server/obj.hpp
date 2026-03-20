#pragma once

#include <Core/ok.hpp>
#include <Core/DBDescriptor.hpp>

#include <SQL/ir.hpp>

namespace xmdb::server {
struct TableObject {
    ok::StringView name;
    ok::Slice<ok::StringView> column_names;
    ok::Slice<SQL::ColumnType> column_types;
};

ok::Slice<TableObject> get_db_table_objects(ok::Allocator *, DBDescriptor *);
} // namespace xmdb::server
