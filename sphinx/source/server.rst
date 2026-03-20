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

Deployment
----------

System requirements
~~~~~~~~~~~~~~~~~~~

* A POSIX-compatible operating system (only Linux tested)
* OpenSSL -- required as a transitive dependency from ``libweb``, even when
  not using HTTPS

Running the server
~~~~~~~~~~~~~~~~~~

After building the project, start the server with:

.. code-block:: shell

                ./build/src/Server/xmdb_server

The server listens for HTTP connections on its configured port.

Storage
~~~~~~~

Each persistent table is stored as a set of files in the working directory
from which the server was launched:

* **`<hash>.sdb`** - record data. Contains table records.
* **`<hash>.xdb`** - B-Tree index. A page-based file that
  stores the primary-key index for the table.
* **`<table>-<column>.idb`** - image data (one per image column).
  Stores image chunks.

``<hash>`` is the hex-encoded SHA-256 digest of the table name.

All files are written relative to the current working directory. There is
currently no configuration option to change the storage path, so make sure to
always start the server from the same directory.

Backups
~~~~~~~

XMDB does not provide a built-in backup mechanism. Backups are fully the
user's responsibility. A few approaches to consider:

* **Filesystem snapshots** - if the underlying filesystem supports snapshots
  (ZFS, Btrfs, LVM), take a snapshot of the directory that contains the
  ``.sdb``, ``.xdb``, and ``.idb`` files. This is the fastest and most
  consistent option.
* **File copy** - stop the server and copy all ``.sdb``, ``.xdb``, and
  ``.idb`` files to a backup location. Stopping the server first avoids
  copying partially-written files.
* **Periodic rsync** - use ``rsync`` to mirror the storage directory to a
  remote host on a schedule. Combining this with a brief server stop ensures
  consistency; without stopping, you may capture an inconsistent state.
* **Block-level replication** - replicate the entire block device or partition
  with tools like DRBD for real-time redundancy.

Regardless of the approach, verify your backups periodically by restoring them
into a test environment.

Scaling
~~~~~~~

XMDB does not support horizontal scaling or replication. The only way to
scale the server is vertically - by adding more CPU, memory, or faster
storage to the machine it runs on.

If the application that talks to the database is bottlenecked by query
latency, consider placing a caching layer (e.g. Redis or Memcached) between
the application and the server to avoid repeated queries for the same data.

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
