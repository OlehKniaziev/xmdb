#!/bin/sh
set -xe
git ls-files | grep "^.*\.[ch]pp$" | etags -
