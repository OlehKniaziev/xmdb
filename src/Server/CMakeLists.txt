add_executable(
  xmdb_server
  main.cpp
  http.cpp
  connection.cpp
  Config.cpp
)

target_link_libraries(xmdb_server PUBLIC LibCore LibSQL::static libweb)
