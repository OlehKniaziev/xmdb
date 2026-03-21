SCENARIOS = [
    {
        "name": "connect_default_db",
        "description": "Connect as admin to the default database",
        "queries": [],
        "expect_connect": True,
    },
    {
        "name": "connect_invalid_db",
        "description": "Connecting to a non-existent database fails",
        "db_name": "nonexistent",
        "queries": [],
        "expect_connect": False,
    },
    {
        "name": "create_table_insert_select",
        "description": "CREATE TABLE, INSERT rows, SELECT them back",
        "queries": [
            {
                "sql": "CREATE TABLE t(id int, name text);",
                "expect_ok": True,
                "expect_rows": None,
            },
            {
                "sql": 'INSERT INTO t(id, name) VALUES (1, "alice"), (2, "bob");',
                "expect_ok": True,
                "expect_rows": None,
            },
            {
                "sql": "SELECT id, name FROM t;",
                "expect_ok": True,
                "expect_rows": [
                    {"id": 1, "name": "alice"},
                    {"id": 2, "name": "bob"},
                ],
            },
        ],
    },
    {
        "name": "create_and_use_database",
        "description": "CREATE DATABASE, USE it, create a table in it",
        "queries": [
            {"sql": "CREATE DATABASE TestDB;", "expect_ok": True},
            {"sql": "USE TestDB;", "expect_ok": True},
            {"sql": "CREATE TABLE items(x int);", "expect_ok": True},
            {"sql": "INSERT INTO items(x) VALUES (42);", "expect_ok": True},
            {"sql": "SELECT x FROM items;", "expect_ok": True, "expect_rows": [{"x": 42}]},
        ],
    },
    {
        "name": "error_on_invalid_sql",
        "description": "Invalid SQL returns an error",
        "queries": [
            {"sql": "INVALID GIBBERISH;", "expect_ok": False},
        ],
    },
    {
        "name": "update_and_verify",
        "description": "INSERT, UPDATE, then SELECT to verify the update took effect",
        "queries": [
            {"sql": "CREATE TABLE u(val int);", "expect_ok": True},
            {"sql": "INSERT INTO u(val) VALUES (10);", "expect_ok": True},
            {"sql": "UPDATE u SET val = 20;", "expect_ok": True},
            {"sql": "SELECT val FROM u;", "expect_ok": True, "expect_rows": [{"val": 20}]},
        ],
    },
    {
        "name": "delete_and_verify",
        "description": "INSERT, DELETE, then SELECT to verify empty",
        "queries": [
            {"sql": "CREATE TABLE d(x int);", "expect_ok": True},
            {"sql": "INSERT INTO d(x) VALUES (1);", "expect_ok": True},
            {"sql": "DELETE FROM d;", "expect_ok": True},
            {"sql": "SELECT x FROM d;", "expect_ok": True, "expect_rows": []},
        ],
    },
]
