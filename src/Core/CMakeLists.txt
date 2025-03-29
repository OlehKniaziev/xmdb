add_library(LibCore
        STATIC
        ok.cpp
        RBTree.hpp
)

set_property(TARGET LibCore PROPERTY POSITION_INDEPENDENT_CODE 1)