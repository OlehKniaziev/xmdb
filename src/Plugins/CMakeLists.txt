file(GLOB PLUGIN_SOURCES "*.c")

foreach(PLUGIN_SOURCE ${PLUGIN_SOURCES})
    get_filename_component(PLUGIN_NAME ${PLUGIN_SOURCE} NAME_WE)
    set(TARGET_NAME "xmdb${PLUGIN_NAME}plugin")

    add_library(${TARGET_NAME} SHARED ${PLUGIN_SOURCE})

    set_target_properties(${TARGET_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        PREFIX "lib"
        OUTPUT_NAME "xmdb${PLUGIN_NAME}plugin"
    )
endforeach()
