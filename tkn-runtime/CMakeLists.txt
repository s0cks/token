project(tkn-runtime CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads)
find_package(uv REQUIRED)
find_package(png REQUIRED)
find_package(gflags REQUIRED)
find_package(cryptopp REQUIRED)
find_package(httpparser REQUIRED)
find_package(leveldb REQUIRED)
find_package(zxing REQUIRED)
find_package(uuid REQUIRED)
find_package(gtest REQUIRED)
find_package(gmock REQUIRED)

add_executable(tkn-runtime
    include/http/http_controller_health.h src/http/http_controller_health.cc
    include/http/http_service_health.h src/http/http_service_health.cc

    include/http/http_controller_pool.h src/http/http_controller_pool.cc
    include/http/http_service_rest.h src/http/http_service_rest.cc

    include/node/node_session_message_handler.h src/node/node_session_message_handler.cc
    include/node/node_session.h
    include/node/node_server.h

    include/peer/peer_queue.h
    include/peer/peer_session.h src/peer/peer_session.cc
    include/peer/peer_session_message_handler.h src/peer/peer_session_message_handler.cc
    include/peer/peer_session_manager.h src/peer/peer_session_manager.cc
    include/peer/peer_session_thread.h src/peer/peer_session_thread.cc

    include/acceptor.h src/acceptor.cc
    include/configuration.h src/configuration.cc
    include/flags.h src/flags.cc
    include/miner.h src/miner.cc
    include/pool.h src/pool.cc
    include/proposal_scope.h src/proposal_scope.cc
    include/proposer.h src/proposer.cc
    include/runtime.h src/runtime.cc

    src/tasks/task_process_output_list.h src/tasks/task_process_input_list.cc
    src/tasks/task_process_input_list.h src/tasks/task_process_output_list.cc
    src/tasks/task_process_transaction.h src/tasks/task_process_transaction.cc

    src/block_builder.h src/block_builder.cc

    main.cc)
target_link_libraries(tkn-runtime
    tkn-platform
    tkn-utils
    tkn-core
    tkn-merkle
    tkn-task-engine
    tkn-rpc
    tkn-http
    tkn-deeplink
    ${CMAKE_THREAD_LIBS_INIT} # pthreads
    ${GFLAGS_LIBRARIES} # gflags
    ${CRYPTOPP_LIBRARIES} # cryptopp
    ${UV_LIBRARIES} # libuv
    ${PNG_LIBRARIES} # libpng
    ${HTTPPARSER_LIBRARIES} # libhttp-parser
    ${LEVELDB_LIBRARIES} # leveldb
    ${ZXING_LIBRARIES} # zxing
    ${UUID_LIBRARIES} # libuuid
)
target_include_directories(tkn-runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)