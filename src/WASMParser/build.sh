#!/usr/bin/sh

set -xe

clang++ -Wall -Wextra -Werror -Os -fno-builtin -fno-rtti -std=c++20 -isystem.. -target wasm32 -nostdlib -Wl,--no-entry -Wl,--export=__wasm_call_ctors -Wl,--export=stbsp_vsnprintf -Wl,--export=sql_parse_source -Wl,--export=module_alloc -Wl,--allow-undefined -o module.wasm wasm_module.cpp
