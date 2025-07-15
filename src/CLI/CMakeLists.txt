add_executable(xmdb
  main.cpp
)

target_link_libraries(xmdb PRIVATE LibCore LibSQL libweb)
