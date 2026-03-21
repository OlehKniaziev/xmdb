add_library(LibCore
  STATIC
  ok.cpp
  DBConnection.cpp
  DBPool.cpp
  DBValue.cpp
  DBDescriptor.cpp
  DBTable.cpp
  DBUser.cpp
  QueryExecutionContext.cpp
  Core.cpp
  util.cpp
  hash.cpp
  BTreeIndex.cpp
  QueryGraph.cpp
  Logger.cpp
  SDFile.cpp
  DBRecord.cpp
  FixedString.cpp
  state.cpp
  image.cpp
  StaticStorage.cpp
  catalog.cpp
)

set_property(TARGET LibCore PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(LibCore PUBLIC libweb)

add_library(LibCore_test STATIC $<TARGET_PROPERTY:LibCore,SOURCES>)
set_property(TARGET LibCore_test PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(LibCore_test PRIVATE XMDB_TEST)
target_include_directories(LibCore_test PUBLIC $<TARGET_PROPERTY:LibCore,INCLUDE_DIRECTORIES>)
target_link_libraries(LibCore_test PUBLIC libweb)
