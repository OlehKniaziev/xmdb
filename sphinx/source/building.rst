Building the project
====================

This page describes how to build the collection of XMDB binaries.


.. contents:: Contents
   :local:
   :depth: 1

----

Prerequisites
-------------

* A POSIX-compatible operating system (only Linux tested, but possibly works on others as well)
* A GNU C, C99 compatible compiler
* A C++ compiler with extensions and C++20 support
* CMake version >= 3.20

Building from the command line
------------------------------

First, fetch the submodules:

.. code-block:: shell

                git submodule init
                git submodule update --recursive

Setup the build directory:

.. code-block:: shell

                cmake -Bbuild

Build the binaries:

.. code-block:: shell

                cmake --build build

The resulting binaries can be found inside ``src/*`` subdirectories.
