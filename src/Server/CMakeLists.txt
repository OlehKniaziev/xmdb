add_executable(xmdb-server main.cpp)
target_link_libraries(xmdb-server xmdb-core xmdb-sql)