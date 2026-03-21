#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <unistd.h>

#include <SQL/SQL.hpp>

#include <Core/Core.hpp>
#include <Core/DBConnection.hpp>
#include <Core/DBPool.hpp>
#include <Core/catalog.hpp>

using namespace ok::literals;
using namespace xmdb;
using namespace xmdb::SQL;

namespace fs = std::filesystem;

class CorePersistenceFixture : public ::testing::Test {
protected:
    void SetUp() override {
        original_dir_ = fs::current_path();

        char tmpl[] = "/tmp/xmdb_integration_XXXXXX";
        char *dir = mkdtemp(tmpl);
        ASSERT_NE(dir, nullptr) << "Failed to create temp directory";

        temp_dir_ = dir;
        ASSERT_EQ(chdir(temp_dir_.c_str()), 0);
    }

    void TearDown() override {
        chdir(original_dir_.c_str());
        fs::remove_all(temp_dir_);
    }

    std::string temp_dir_;
    fs::path original_dir_;
};

TEST_F(CorePersistenceFixture, create_table_persists_in_catalog) {
    ok::ArenaAllocator arena{};
    QueryResults results{};
    String error{};

    // First session: create a table.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};

        bool ok = compile_and_execute_source(&arena, &conn, "CREATE TABLE persisted(id int, name text);"_sv, &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
    }

    // Second session: reload and verify the table exists.
    {
        DBPool pool{&arena, 0};
        DBDescriptor *default_db = pool.get_db("default"_sv);

        bool found = false;
        for (UZ table_idx = 0; table_idx < default_db->tables.count; ++table_idx) {
            if (default_db->tables[table_idx]->name() == "persisted"_sv) {
                found = true;
                ASSERT_EQ(default_db->tables[table_idx]->columns_count(), 2);
                break;
            }
        }
        ASSERT_TRUE(found) << "Table 'persisted' was not found after reload";
    }
}

TEST_F(CorePersistenceFixture, insert_data_persists_across_restarts) {
    ok::ArenaAllocator arena{};
    QueryResults results{};
    String error{};

    // First session: create table, insert data.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};

        bool ok = compile_and_execute_source(&arena, &conn,
            R"sql(CREATE TABLE data_test(id int, label text);
                  INSERT INTO data_test(id, label) VALUES (1, "alpha"), (2, "beta");)sql"_sv,
            &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
    }

    // Second session: reload and select data.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};
        populate_ir_context_from_pool(&conn.ir_ctx, &pool);

        bool ok = compile_and_execute_source(&arena, &conn,
            "SELECT id, label FROM data_test;"_sv,
            &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
        ASSERT_TRUE(results.value.has_value());

        DBTable *table = results.value.get();
        ASSERT_EQ(table->rows_count(), 2);

        DBTableOutlet outlet{table};
        DBTableStream id_stream = outlet.column_stream(&arena, 0);

        Optional<Value> val = id_stream.next();
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(val.get().as_int(), 1);

        val = id_stream.next();
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(val.get().as_int(), 2);

        ASSERT_FALSE(id_stream.next());

        DBTableStream label_stream = outlet.column_stream(&arena, 1);

        val = label_stream.next();
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(val.get().as_string(), "alpha"_sv);

        val = label_stream.next();
        ASSERT_TRUE(val.has_value());
        ASSERT_EQ(val.get().as_string(), "beta"_sv);

        ASSERT_FALSE(label_stream.next());

    }
}

TEST_F(CorePersistenceFixture, multiple_databases_persist) {
    ok::ArenaAllocator arena{};
    QueryResults results{};
    String error{};

    // First session: create two databases with tables.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};

        bool ok = compile_and_execute_source(&arena, &conn,
            R"sql(CREATE DATABASE DbOne;
                  USE DbOne;
                  CREATE TABLE t1(x int);)sql"_sv,
            &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
    }

    // Second session: verify both 'default' and 'DbOne' exist.
    {
        DBPool pool{&arena, 0};

        bool found_default = false;
        bool found_db_one = false;

        for (DBDescriptor *db = pool.db_descriptors; db != nullptr; db = db->next) {
            if (db->name == "default"_sv) found_default = true;
            if (db->name == "DbOne"_sv) {
                found_db_one = true;
                bool has_t1 = false;
                for (UZ table_idx = 0; table_idx < db->tables.count; ++table_idx) {
                    if (db->tables[table_idx]->name() == "t1"_sv) {
                        has_t1 = true;
                    }
                }
                ASSERT_TRUE(has_t1) << "Table 't1' not found in DbOne";
            }
        }

        ASSERT_TRUE(found_default) << "'default' database missing after reload";
        ASSERT_TRUE(found_db_one) << "'DbOne' database missing after reload";
    }
}

TEST_F(CorePersistenceFixture, drop_table_removes_from_catalog) {
    ok::ArenaAllocator arena{};
    QueryResults results{};
    String error{};

    // First session: create then drop a table.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};

        bool ok = compile_and_execute_source(&arena, &conn,
            "CREATE TABLE to_drop(a int); DROP TABLE to_drop;"_sv,
            &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
    }

    // Second session: verify the table is gone.
    {
        DBPool pool{&arena, 0};
        DBDescriptor *default_db = pool.get_db("default"_sv);

        for (UZ table_idx = 0; table_idx < default_db->tables.count; ++table_idx) {
            ASSERT_NE(default_db->tables[table_idx]->name(), "to_drop"_sv)
                << "Dropped table 'to_drop' still present after reload";
        }
    }
}

TEST_F(CorePersistenceFixture, create_user_persists) {
    ok::ArenaAllocator arena{};
    QueryResults results{};
    String error{};

    // First session: create a user.
    {
        DBPool pool{&arena, 0};
        DBUser admin = DBUser::admin();
        DBDescriptor *default_db = pool.get_db("default"_sv);
        DBConnection conn{&pool, default_db, &admin};

        bool ok = compile_and_execute_source(&arena, &conn,
            "CREATE USER test_user;"_sv,
            &results, &error);
        ASSERT_TRUE(ok) << error.cstr();
    }

    // Second session: verify the user exists.
    {
        DBPool pool{&arena, 0};
        DBDescriptor *default_db = pool.get_db("default"_sv);

        Optional<DBUser *> user = default_db->find_user("test_user"_sv);
        ASSERT_TRUE(user.has_value()) << "User 'test_user' not found after reload";
    }
}
