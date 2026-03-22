cmake_minimum_required(VERSION 3.20)

project(xmdb VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM src)

add_compile_options(-Wall -Wextra -Werror -Wno-c99-designator -g -O0)

add_subdirectory(third_party)

add_subdirectory(src/Core)
add_subdirectory(src/Plugin)
add_subdirectory(src/ArgParser)
add_subdirectory(src/SQL)
add_subdirectory(src/Server)
add_subdirectory(src/CLI)
# TODO(oleh): Build this only if in developer mode.
add_subdirectory(src/IR)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
)

FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB test_files "tests/*-test.cpp")
list(FILTER test_files EXCLUDE REGEX ".*-integration-test\\.cpp$")

include(GoogleTest)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

foreach(test_file ${test_files})
  string(REGEX REPLACE "^.*/tests/(.*)\\.cpp$" "\\1" test_exe_name ${test_file})

  add_executable(
    ${test_exe_name}
    ${test_file}
  )

  target_compile_definitions(${test_exe_name} PRIVATE XMDB_TEST)

  target_link_libraries(
    ${test_exe_name}
    GTest::gtest_main
  )

  target_link_libraries(
    ${test_exe_name}
    LibCore_test
    LibSQL::shared
  )

  gtest_discover_tests(${test_exe_name})
endforeach()

# Integration tests: link against LibCore (not LibCore_test) so XMDB_TEST is
# NOT defined and DBPool is non-ephemeral.
file(GLOB integration_test_files "tests/*-integration-test.cpp")

# Plugin integration test is self-contained: the same source file is compiled
# as a shared library (the plugin under test) and as the GTest executable.
list(REMOVE_ITEM integration_test_files "${CMAKE_SOURCE_DIR}/tests/Plugin-integration-test.cpp")

add_library(PluginIntegrationTestPlugin SHARED tests/Plugin-integration-test.cpp)
target_compile_definitions(PluginIntegrationTestPlugin PRIVATE XMDB_BUILDING_TEST_PLUGIN)

add_executable(Plugin-integration-test tests/Plugin-integration-test.cpp)
target_compile_definitions(Plugin-integration-test PRIVATE
    XMDB_TEST_PLUGIN_PATH="$<TARGET_FILE:PluginIntegrationTestPlugin>")
target_link_libraries(Plugin-integration-test GTest::gtest_main LibPlugin LibCore)
gtest_discover_tests(Plugin-integration-test)

foreach(test_file ${integration_test_files})
  string(REGEX REPLACE "^.*/tests/(.*)\\.cpp$" "\\1" test_exe_name ${test_file})

  add_executable(${test_exe_name} ${test_file})

  target_link_libraries(${test_exe_name} GTest::gtest_main)
  target_link_libraries(${test_exe_name} LibCore LibSQL::shared)

  gtest_discover_tests(${test_exe_name})
endforeach()

# TODO(oleh): Make this executed only when building tests.
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
  add_test(
    NAME integration-http
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
      -s ${CMAKE_SOURCE_DIR}/tests
      -t ${CMAKE_SOURCE_DIR}/tests
      -p "test_http.py"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  set_tests_properties(integration-http PROPERTIES
    ENVIRONMENT "SERVER_BIN=$<TARGET_FILE:xmdb_server>;CLI_BIN=$<TARGET_FILE:xmdb>"
  )

  add_test(
    NAME integration-cli
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
      -s ${CMAKE_SOURCE_DIR}/tests
      -t ${CMAKE_SOURCE_DIR}/tests
      -p "test_cli.py"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  set_tests_properties(integration-cli PROPERTIES
    ENVIRONMENT "SERVER_BIN=$<TARGET_FILE:xmdb_server>;CLI_BIN=$<TARGET_FILE:xmdb>"
  )
endif()

file(GLOB snap_files "tests/snaps/*.cpp")

set(snap "./tests/snap.py")
set(snapdir "./tests/snaps")

cmake_path(ABSOLUTE_PATH snap)
cmake_path(ABSOLUTE_PATH snapdir)

foreach(snap_file ${snap_files})
  string(REGEX REPLACE "^.*/snaps/(.*)\\.cpp$" "\\1-snap" snap_exe_name ${snap_file})

  add_executable(${snap_exe_name} ${snap_file})
  add_test(NAME ${snap_exe_name} COMMAND ${snap} "-f" "-d" ${snapdir} ${snap_exe_name})

  target_compile_definitions(${snap_exe_name} PRIVATE XMDB_TEST)

  target_link_libraries(
    ${snap_exe_name}
    LibCore_test
    LibSQL::shared
  )
endforeach()
