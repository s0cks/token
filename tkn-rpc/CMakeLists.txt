project(tkn-rpc CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Protobuf REQUIRED)
include_directories(protos)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
    PROTO_SOURCES PROTO_HEADERS
    protos/message_version.proto
    protos/message_verack.proto
    protos/message_paxos.proto
)

add_library(tkn-rpc STATIC
    ${PROTO_HEADERS} ${PROTO_SOURCES}

    include/rpc/rpc_common.h
    include/rpc/rpc_message.h src/rpc/rpc_message.cc
    include/rpc/rpc_message_accepted.h src/rpc/rpc_message_accepted.cc
    include/rpc/rpc_message_block.h
    include/rpc/rpc_message_commit.h src/rpc/rpc_message_commit.cc
    include/rpc/rpc_message_handler.h
    include/rpc/rpc_message_handshake.h
    include/rpc/rpc_message_object.h
    include/rpc/rpc_message_paxos.h
    include/rpc/rpc_message_prepare.h src/rpc/rpc_message_prepare.cc
    include/rpc/rpc_message_promise.h src/rpc/rpc_message_promise.cc
    include/rpc/rpc_message_rejected.h src/rpc/rpc_message_rejected.cc
#    include/rpc/rpc_message_transaction.h src/rpc/rpc_message_transaction.cc
    include/rpc/rpc_message_verack.h
        include/rpc/rpc_message_version.h
        include/rpc/rpc_messages.h
    include/rpc/rpc_session.h)
target_link_libraries(tkn-rpc tkn-utils tkn-core protobuf::libprotobuf)
target_include_directories(tkn-rpc PUBLIC ${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})