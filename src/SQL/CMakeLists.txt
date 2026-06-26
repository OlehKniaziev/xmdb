add_library(LibSQL
  OBJECT
  ast.cpp
  Lexer.cpp
  Parser.cpp
  constructor_linux.cpp
  ir.cpp
  type_check.cpp
  type_check2.cpp
  SQL.cpp
)

set_property(TARGET LibSQL PROPERTY POSITION_INDEPENDENT_CODE 1)

target_link_libraries(LibSQL PUBLIC LibCore)

add_library(LibSQL_shared SHARED $<TARGET_OBJECTS:LibSQL>)
add_library(LibSQL_static STATIC $<TARGET_OBJECTS:LibSQL>)

target_link_libraries(LibSQL_shared PUBLIC LibCore)
target_link_libraries(LibSQL_static PUBLIC LibCore)

add_library(LibSQL::shared ALIAS LibSQL_shared)
add_library(LibSQL::static ALIAS LibSQL_static)
