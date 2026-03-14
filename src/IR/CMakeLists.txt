add_executable(
  xmdb_ir
  main.cpp
)

target_link_libraries(xmdb_ir PUBLIC LibCore LibSQL::shared LibArgParser)
