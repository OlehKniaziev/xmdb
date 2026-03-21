Development guide
=================

This page covers how to run and extend the XMDB test suite.
For instructions on building the project first, see :doc:`building`.

.. contents:: Contents
   :local:
   :depth: 1

----

Running tests
-------------

The recommended way to run the full test suite is through the ``scripts/test``
helper, which builds the project, runs all snapshot tests, and then invokes
``ctest``:

.. code-block:: shell

   ./scripts/test

You can also drive CTest directly if the project is already built:

.. code-block:: shell

   # Run the full suite
   ctest --test-dir build

   # Filter by name pattern
   ctest --test-dir build -R <pattern>

   # Show test output
   ctest --test-dir build -V

Or run a single test executable without CTest:

.. code-block:: shell

   ./build/tests/<test-name>    # e.g. ./build/tests/SQLParser-test

----

How the test system works in CMake
-----------------------------------

Tests are registered in the root ``CMakeLists.txt`` and split into two
categories.

**Unit tests** (``tests/*-test.cpp``)

Those should exercise individual functions/features/code paths, one file per class or system component.

**Snapshot tests** (``tests/snaps/*.cpp``)

Those are often also called 'regression tests'.

----

Unit tests
----------

Unit tests live in ``tests/`` and use the
`GoogleTest <https://github.com/google/googletest>`_ framework, which is
fetched automatically at configure time via ``FetchContent`` — no manual
installation is required.

Adding a new test is pretty straighforward. Look at the examples from the ``test`` directory,
or consult gtest documentation for more advanced usage.

----

Snapshot tests
--------------

Snapshot tests live under ``tests/snaps/``. Each snapshot test is a small
standalone C++ program whose exit code, stdout,
and stderr are captured and compared against a JSON file stored alongside it.

**Snapshot file format**

Each snapshot is stored as ``<Name>-snap.json``:

.. code-block:: json

   {
     "code":   0,
     "stdout": "...",
     "stderr": ""
   }

The snapshot runner is ``tests/snap.py``. When run without the ``-f`` flag provided
it prints any diff and prompts interactively whether to accept the new output as the
updated baseline. With the ``-f`` present, it exits with code 1 immediately on any
mismatch, which is the mode used in CI.

**Adding a new snapshot test**

1. Create ``tests/snaps/MyTest.cpp`` with a ``main()`` that exercises the
   feature and prints the expected output.
2. Rebuild: ``cmake --build build``.
3. Run the runner once without ``-f`` to record the initial snapshot:

   .. code-block:: shell

      cd build && ../tests/snap.py -d ../tests/snaps ./MyTest-snap

4. Commit both ``MyTest.cpp`` and the generated ``MyTest-snap.json``.

----

Integration tests
-----------------

Integration tests verify that the major components of XMDB work correctly
together. Unlike unit tests, which exercise individual functions in isolation,
integration tests operate against a fully initialised system - writing to disk,
spawning the server, and communicating over the network.

**C++ integration tests** (``tests/*-integration-test.cpp``)

These use GoogleTest and follow the same conventions as unit tests. The key
difference is that they are linked against ``LibCore`` instead of
``LibCore_test``, so the ``XMDB_TEST`` define is absent and the database
writes to disk. To add a new C++ integration test, create a file matching the
``*-integration-test.cpp`` pattern in ``tests/`` - CMake picks it up
automatically.

**Python integration tests** (``tests/integration/``)

These start a real server process and exercise it through the HTTP API and the
CLI. Test scenarios are defined as data in ``tests/integration/scenarios.py``;
both the HTTP and CLI test suites consume them, so adding a new scenario to
that file automatically creates tests for both interfaces.

To add a scenario, append a dictionary to the ``SCENARIOS`` list in
``scenarios.py``. Each scenario has a name, a list of SQL queries to execute,
and optional expected rows. See the existing entries for the format.

----

Development process
--------------------

All changes go through the following workflow:

1. **Issue** - create a GitHub issue describing the bug, feature, or task.
2. **Pull request** - open a PR against the ``development`` branch with the
   proposed changes. Reference the issue in the PR description.
3. **Review** - at least one other contributor reviews the PR and leaves
   feedback or approval.
4. **Merge to development** - once approved, the PR is merged into
   ``development``.
5. **Merge to production** - after the features on ``development`` have been
   tested, ``development`` is merged into ``production``.

----

Release process
---------------

XMDB uses `Semantic Versioning <https://semver.org/>`_ (SemVer) for version
numbers.

There is no fixed release schedule. A new release is cut when enough
important changes or fixes have accumulated to justify one.

Developing the GUI
------------------

See the :doc:`gui` docs for instructions.
