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
        BTree.cpp
)

set_property(TARGET LibCore PROPERTY POSITION_INDEPENDENT_CODE 1)
