#!/bin/sh

set -xe

doxygen

XMDB_ROOT=$(pwd) sphinx-build -M html ./sphinx/source ./docs
