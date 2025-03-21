add_library(xmdb-sql
        SHARED
        ast.cpp
        Lexer.cpp
        Parser.cpp
        constructor_linux.cpp)

target_link_libraries(xmdb-sql xmdb-core)