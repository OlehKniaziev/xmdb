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
)

set_property(TARGET LibCore PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(LibCore PUBLIC libweb)
