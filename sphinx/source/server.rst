Using the Server
======

This page describes the XMDB server and its API.

The server allows you to expose your database to the network.
For now, only the HTTP(S) protocol is supported, but a custom
application layer protocol is planned to be implemented in the future.

The choice of HTTP as a primary network protocol might look
weird, unconventional or unsafe, but there are some advantages of using it.

1. Simplicity - implemnting a custom application layer network procotocol
   was never the focus of the project. HTTP is very simple to get started with.
2. Frontend use - first-class support for querying the database straight from
   the browser is a big advantage, considering the media-first focus of the
   XMDB project. No proxies needed!

.. contents:: Contents
   :local:
   :depth: 2

----

Running the server
------------------

After building the project, start the server with:

.. code-block:: shell

                ./build/src/Server/xmdb_server

The server listens for HTTP connections on its configured port.

----

API
---

HTTP API
~~~~~~~~

POST /connect
^^^^^^^^^^^^^

Authenticates a user against a database and returns a connection ID for use
in subsequent requests.

**Request body**

.. list-table::
   :header-rows: 1

   * - Field
     - Type
     - Description
   * - ``username``
     - ``string``
     - The username to authenticate as.
   * - ``password_hash``
     - ``string``
     - SHA-256 digest of the user's password, encoded as a hexadecimal string.
   * - ``db_name``
     - ``string``
     - Name of the database to connect to.

**Response**

On success the response body is the connection ID as a plain integer (not
JSON). Use this value in subsequent requests.

**Errors**

.. list-table::
   :header-rows: 1

   * - Status
     - Condition
   * - ``400``
     - Missing or malformed fields, invalid hex string, or incorrect password.
   * - ``404``
     - Database or user not found.

**Example**

.. code-block:: json

   {
     "username": "admin",
     "password_hash": "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
     "db_name": "mydb"
   }

POST /run-query
^^^^^^^^^^^^^^^

Executes a SQL query on an existing connection and returns the results.

**Request body**

.. list-table::
   :header-rows: 1

   * - Field
     - Type
     - Description
   * - ``connection_id``
     - ``number``
     - The connection ID obtained from ``/connect``.
   * - ``query``
     - ``string``
     - The SQL query to execute.

**Response**

The response is a JSON object. On success:

.. code-block:: json

   {
     "ok": true,
     "column_names": ["id", "name"],
     "column_types": ["INTEGER", "TEXT"],
     "rows": [
       {"id": 1, "name": "Alice"},
       {"id": 2, "name": "Bob"}
     ]
   }

If the query succeeds but produces no result set (e.g. ``INSERT``), the
``column_names``, ``column_types``, and ``rows`` fields are omitted.

On failure:

.. code-block:: json

   {
     "ok": false,
     "error_message": "description of the error"
   }

**Value encoding**

.. list-table::
   :header-rows: 1

   * - Column type
     - JSON encoding
   * - ``INTEGER``
     - ``number``
   * - ``TEXT``
     - ``string``
   * - ``BOOLEAN``
     - ``true`` / ``false``
   * - ``IMAGE_CHUNK``
     - Object with ``x``, ``y``, ``width``, ``height`` (numbers) and ``data``
       (hex string).

**Errors**

.. list-table::
   :header-rows: 1

   * - Status
     - Condition
   * - ``400``
     - Missing fields, non-integral connection ID, unknown connection, or
       query compilation/execution failure (returned inside JSON with
       ``"ok": false``).

POST /get-db-objects
^^^^^^^^^^^^^^^^^^^^

Returns the list of tables and their schemas for the database associated with
the given connection.

**Request body**

.. list-table::
   :header-rows: 1

   * - Field
     - Type
     - Description
   * - ``connection_id``
     - ``number``
     - The connection ID obtained from ``/connect``.

**Response**

.. code-block:: json

   {
     "ok": true,
     "tables": [
       {
         "name": "users",
         "column_names": ["id", "name", "email"],
         "column_types": ["INTEGER", "TEXT", "TEXT"]
       }
     ]
   }

Each entry in ``tables`` describes one table with its column names and types.

**Errors**

.. list-table::
   :header-rows: 1

   * - Status
     - Condition
   * - ``400``
     - Missing ``connection_id`` field or unknown connection.
