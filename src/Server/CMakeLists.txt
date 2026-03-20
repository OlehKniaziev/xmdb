add_executable(
  xmdb_server
  main.cpp
  http.cpp
  connection.cpp
  Config.cpp
  obj.cpp
)

target_link_libraries(xmdb_server PUBLIC LibCore LibSQL::static LibArgParser libweb)
