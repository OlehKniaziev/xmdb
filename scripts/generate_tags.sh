#!/bin/sh
set -xe
git ls-files --recurse-submodules | grep '^.*\.[ch]\(pp\)\?$' | xargs etags
