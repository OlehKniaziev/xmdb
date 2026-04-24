#include "Core.hpp"

#include <SQL/SQL.hpp>
#include <SQL/util.hpp>

namespace xmdb
{
bool compile_and_execute_source(ok::ArenaAllocator *arena,
                                DBConnection *connection, StringView source,
                                QueryResults *results, String *error)
{
    SQL::TypedCompiledQuery typed_query{};
    connection->ir_ctx.source = source;
    if (!compile_and_type_check_source(arena, source, &typed_query, error,
                                       &connection->ir_ctx))
    {
        return false;
    }

    connection->execute(&typed_query, results);
    if (results->error.has_value())
    {
        ErrorWithSourceLocation err = results->error.value;
        *error = SQL::format_error(arena, err.location, err.message.view());
        return false;
    }

    return true;
}

void populate_ir_context_from_pool(SQL::IrContext *ctx, DBPool *pool)
{
    ctx->database_schemas.count = 0;
    ctx->error = {};

    for (DBDescriptor *db = pool->db_descriptors; db != nullptr; db = db->next)
    {
        SQL::DBSchema db_schema =
                SQL::DBSchema::alloc(ctx->allocator, db->name);

        for (UZ table_idx = 0; table_idx < db->tables.count; ++table_idx)
        {
            DBTable *table = db->tables[table_idx];
            ok::String table_name = table->name().to_string(ctx->allocator);
            SQL::TableSchema *table_schema =
                    db_schema.alloc_table_schema(table_name, true);

            ok::Slice<ok::StringView> column_names = table->columns_names();
            ok::Slice<SQL::ColumnType> column_types = table->columns_types();

            ok::List<SQL::ColumnType> &schema_column_types =
                    table_schema->columns_types.get();

            for (UZ col_idx = 0; col_idx < table->columns_count(); ++col_idx)
            {
                ok::String column_name =
                        column_names[col_idx].to_string(ctx->allocator);
                SQL::ColumnType column_type = column_types[col_idx];

                table_schema->columns_names.push(column_name);
                schema_column_types.push(column_type);
            }
        }

        ctx->database_schemas.push(db_schema);
    }
}
} // namespace xmdb
