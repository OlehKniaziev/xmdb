add_library(LibSQL
  OBJECT
  ast.cpp
  Lexer.cpp
  Parser.cpp
  constructor_linux.cpp
  ir.cpp
  type_check.cpp
)

set_property(TARGET LibSQL PROPERTY POSITION_INDEPENDENT_CODE 1)

link_libraries(LibCore)

add_library(LibSQL_shared SHARED $<TARGET_OBJECTS:LibSQL>)
add_library(LibSQL_static STATIC $<TARGET_OBJECTS:LibSQL>)

add_library(LibSQL::shared ALIAS LibSQL_shared)
add_library(LibSQL::static ALIAS LibSQL_static)
