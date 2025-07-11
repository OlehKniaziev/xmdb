add_executable(xmdb_server main.cpp)

target_link_libraries(xmdb_server PUBLIC LibSQL::static LibCore libweb)
