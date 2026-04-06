add_library(xmdbmediaplugin SHARED media.cpp)

find_package(PkgConfig REQUIRED)

pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0)

set_target_properties(xmdbmediaplugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        PREFIX "lib"
        OUTPUT_NAME xmdbmediaplugin
)

target_link_libraries(xmdbmediaplugin
        PkgConfig::gstreamer
        PkgConfig::gstreamer-app
        libweb
)
