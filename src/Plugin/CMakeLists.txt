add_library(LibPlugin
  STATIC
  NativeLibrary.cpp
  Plugin.cpp
  PluginManager.cpp
)

set_property(TARGET LibPlugin PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(LibPlugin PUBLIC LibCore)
