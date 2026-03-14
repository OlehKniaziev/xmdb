add_library(LibArgParser
  STATIC
  ArgParser.cpp)

set_property(TARGET LibArgParser PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(LibArgParser PUBLIC LibCore)
