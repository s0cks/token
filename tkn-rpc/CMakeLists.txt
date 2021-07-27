project(tkn-rpc CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-rpc STATIC
    include/rpc/rpc_common.h
    include/rpc/rpc_message.h src/rpc/rpc_message.cc
    include/rpc/rpc_message_accepted.h src/rpc/rpc_message_accepted.cc
#    include/rpc/rpc_message_block.h src/rpc/rpc_message_block.cc
    include/rpc/rpc_message_commit.h src/rpc/rpc_message_commit.cc
    include/rpc/rpc_message_handler.h
    include/rpc/rpc_message_handshake.h
    include/rpc/rpc_message_object.h
    include/rpc/rpc_message_paxos.h
    include/rpc/rpc_message_prepare.h src/rpc/rpc_message_prepare.cc
    include/rpc/rpc_message_promise.h src/rpc/rpc_message_promise.cc
    include/rpc/rpc_message_rejected.h src/rpc/rpc_message_rejected.cc
#    include/rpc/rpc_message_transaction.h src/rpc/rpc_message_transaction.cc
    include/rpc/rpc_message_verack.h src/rpc/rpc_message_verack.cc
    include/rpc/rpc_message_version.h src/rpc/rpc_message_version.cc
    include/rpc/rpc_messages.h
    include/rpc/rpc_session.h)
target_link_libraries(tkn-rpc tkn-utils tkn-core)
target_include_directories(tkn-rpc PUBLIC ${PROJECT_SOURCE_DIR}/include)