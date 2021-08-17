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

#cmake_minimum_required(VERSION 3.0)
#project(GTestProject)
#
#include(ExternalProject)
#
#set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
#
#ExternalProject_Add(googletest
#  GIT_REPOSITORY https://github.com/google/googletest
#  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
#)
#
#include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
#link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
#
#add_executable(FirstTest main.cpp)
#add_dependencies(FirstTest googletest)
#target_link_libraries(FirstTest gtest gtest_main pthread)

add_executable(tkn-runtime
  src/commit/block_committer.h
  src/commit/block_committer_async.h src/commit/block_committer_async.cc
  src/commit/block_committer_sync.h src/commit/block_committer_sync.cc

  include/consensus/proposal_state.h src/consensus/proposal_state.cc
  include/consensus/acceptor.h src/consensus/acceptor.cc
  include/consensus/election.h src/consensus/election.cc
  include/consensus/proposal_listener.h
  include/consensus/proposal_timer.h src/consensus/proposal_timer.cc
  include/consensus/proposer.h src/consensus/proposer.cc

  include/http/http_controller_health.h src/http/http_controller_health.cc
  include/http/http_service_health.h src/http/http_service_health.cc
  include/http/http_controller_info.h src/http/http_controller_info.cc
  include/http/http_controller_pool.h src/http/http_controller_pool.cc
  include/http/http_service_rest.h src/http/http_service_rest.cc

  include/node/node_session_message_handler.h src/node/node_session_message_handler.cc
  include/node/node_session.h src/node/node_session.cc
  include/node/node_server.h src/node/node_server.cc

  include/peer/peer_queue.h
  include/peer/peer_session.h src/peer/peer_session.cc
  include/peer/peer_session_message_handler.h src/peer/peer_session_message_handler.cc
  include/peer/peer_session_manager.h src/peer/peer_session_manager.cc
  include/peer/peer_session_thread.h src/peer/peer_session_thread.cc

  src/verifier/block_verifier.h
  src/verifier/block_verifier_sync.h src/verifier/block_verifier_sync.cc

  include/batch.h

  include/config.h src/config.cc
  include/flags.h src/flags.cc
  include/miner.h src/miner.cc
  include/miner_listener.h
  include/object_pool.h src/object_pool.cc
  include/runtime.h src/runtime.cc
  src/block_builder.h src/block_builder.cc

  main.cc src/verifier/block_verifier_async.cc src/verifier/block_verifier_async.h src/verifier/input_verifier.h src/tasks/task_verify_input.h src/tasks/task_verify_input.cc src/tasks/task_verify_transaction.cc src/tasks/task_verify_transaction.h src/tasks/task_verify_output.cc src/tasks/task_verify_output.h src/verifier/verifier_output.h src/tasks/task_verify_transaction_inputs.cc src/tasks/task_verify_transaction_inputs.h src/tasks/task_verify_transaction_objects.h src/tasks/task_verify_transaction_outputs.cc src/tasks/task_verify_transaction_outputs.h)
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
target_include_directories(tkn-runtime PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(tkn-runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)