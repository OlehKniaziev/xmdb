find_program(CLANGXX clang++)

if(NOT CLANGXX)
    message(STATUS "clang++ not found -- skipping WASMParser target")
    return()
endif()

set(WASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/wasm_module.cpp)
set(WASM_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/module.wasm)

add_custom_command(
    OUTPUT ${WASM_OUTPUT}
    COMMAND ${CLANGXX}
        -Wall -Wextra -Werror
        -Os -fno-builtin -fno-rtti -std=c++20
        -isystem ${CMAKE_SOURCE_DIR}/src
        -target wasm32 -nostdlib
        -Wl,--no-entry
        -Wl,--export=__wasm_call_ctors
        -Wl,--export=stbsp_vsnprintf
        -Wl,--export=sql_parse_source
        -Wl,--export=module_alloc
        -Wl,--allow-undefined
        -o ${WASM_OUTPUT}
        ${WASM_SOURCE}
    DEPENDS ${WASM_SOURCE}
    COMMENT "Building WASM parser module"
    VERBATIM
)

add_custom_target(WASMParser ALL
    DEPENDS ${WASM_OUTPUT}
)
