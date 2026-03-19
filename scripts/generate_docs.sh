#!/bin/sh

set -e

if test "$#" -lt "1"; then
    echo "Not enough arguments"
    exit 1
fi

if [ "$1" == "html" ]; then
    doxygen

    XMDB_ROOT=$(pwd) sphinx-build -M html ./sphinx/source ./docs
elif [ "$1" == "pdf" ]; then
    doxygen

    XMDB_ROOT=$(pwd) sphinx-build -M latex ./sphinx/source ./latex-docs
    pushd ./latex-docs/latex
    make
    cp ./xmdb.pdf ../../
    popd

    echo "Generated xmdb.pdf in the current directory"
else
    echo "Uncrecognized target format '$1'"
    exit 1
fi
