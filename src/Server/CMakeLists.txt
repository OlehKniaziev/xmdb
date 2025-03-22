add_executable(xmdb_server main.cpp)
target_link_libraries(xmdb_server LibSQL::static LibCore)