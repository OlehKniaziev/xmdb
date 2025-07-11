add_custom_command(
  OUTPUT libweb.a
  COMMAND bash ./build.sh
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libweb"
)

add_custom_target(build_libweb DEPENDS libweb.a)

add_library(libweb STATIC IMPORTED GLOBAL)
add_dependencies(libweb build_libweb)

set_target_properties(libweb PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libweb/libweb.a")
target_include_directories(libweb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libweb/src")
