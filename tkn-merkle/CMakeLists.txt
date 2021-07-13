project(tkn-merkle CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-merkle STATIC
        include/merkle/node.h
        include/merkle/proof.h
        include/merkle/tree.h src/merkle/tree.cc)
target_include_directories(tkn-merkle PUBLIC ${PROJECT_SOURCE_DIR}/include)