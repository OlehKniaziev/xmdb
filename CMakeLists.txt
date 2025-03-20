cmake_minimum_required(VERSION 3.10)

project(xmdb VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(XMDB_LIB_SOURCES
        src/SQLLexer.cpp
        src/ok.cpp
        src/constructor_linux.cpp
        src/SQLParser.cpp
        src/SQLParser.hpp
        src/ast.cpp
        src/ast.hpp
)

set(XMDB_SOURCES src/main.cpp)

set(XMDB_SRC_DIR src)

add_library(xmdb-core SHARED ${XMDB_LIB_SOURCES})

add_executable(xmdb ${XMDB_SOURCES})

target_link_libraries(xmdb xmdb-core)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
)

FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB test_files "tests/*-test.cpp")

include(GoogleTest)

foreach(test_file ${test_files})
  string(REGEX REPLACE "^.*/tests/(.*)\\.cpp$" "\\1" test_exe_name ${test_file})

  add_executable(
    ${test_exe_name}
    ${test_file}
  )

  target_include_directories(
    ${test_exe_name}
    PUBLIC
    ${XMDB_SRC_DIR}
  )

  target_link_libraries(
    ${test_exe_name}
    GTest::gtest_main
  )

  target_link_libraries(
    ${test_exe_name}
    xmdb-core
  )

  gtest_discover_tests(${test_exe_name})

endforeach()

# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
