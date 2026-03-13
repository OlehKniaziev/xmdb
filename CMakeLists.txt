cmake_minimum_required(VERSION 3.20)

project(xmdb VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM src)

add_compile_options(-Wall -Wextra -Werror -g -O0)

add_subdirectory(third_party)

add_subdirectory(src/Core)
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

include(GoogleTest)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

foreach(test_file ${test_files})
  string(REGEX REPLACE "^.*/tests/(.*)\\.cpp$" "\\1" test_exe_name ${test_file})

  add_executable(
    ${test_exe_name}
    ${test_file}
  )

  target_link_libraries(
    ${test_exe_name}
    GTest::gtest_main
  )

  target_link_libraries(
    ${test_exe_name}
    LibCore
    LibSQL::shared
  )

  gtest_discover_tests(${test_exe_name})
endforeach()

file(GLOB snap_files "tests/snaps/*.cpp")

set(snap "./tests/snap.py")
set(snapdir "./tests/snaps")

cmake_path(ABSOLUTE_PATH snap)
cmake_path(ABSOLUTE_PATH snapdir)

foreach(snap_file ${snap_files})
  string(REGEX REPLACE "^.*/snaps/(.*)\\.cpp$" "\\1-snap" snap_exe_name ${snap_file})

  add_executable(${snap_exe_name} ${snap_file})
  add_test(NAME ${snap_exe_name} COMMAND ${snap} "-f" "-d" ${snapdir} ${snap_exe_name})

  target_link_libraries(
    ${snap_exe_name}
    LibCore
    LibSQL::shared
  )
endforeach()
