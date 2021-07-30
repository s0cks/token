project(tkn-protos CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Protobuf REQUIRED)

include_directories(protos)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
    TKN_PROTO_SOURCES TKN_PROTO_HEADERS
    protos/input.proto
    protos/output.proto
    protos/transaction.proto
    protos/block.proto
    protos/unclaimed_transaction.proto
)

add_library(tkn-protos ${TKN_PROTO_HEADERS} ${TKN_PROTO_SOURCES})
target_include_directories(tkn-protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tkn-protos protobuf::libprotobuf)