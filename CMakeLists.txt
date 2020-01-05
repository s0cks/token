cmake_minimum_required(VERSION 3.7)
project("token" CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(GRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(Threads)
find_package(glog REQUIRED)
find_package(LevelDB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(gflags REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${SQLite3_INCLUDE_DIRS}) # TODO: Remove
include_directories(${CRYPTO++_INCLUDE_DIR})
include_directories(include)
include_directories(src)

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
file(GLOB_RECURSE TOKEN_LIB_SOURCES include/*.h src/*.cc src/*.h)
file(GLOB_RECURSE TOKEN_PROTO_SOURCES protos/*.proto)
file(GLOB_RECURSE TOKEN_SERVICE_PROTO_SOURCES protos/*.proto)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${TOKEN_PROTO_SOURCES})
# grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${CMAKE_BINARY_DIR} ${TOKEN_SERVICE_PROTO_SOURCES})

#=================================================
#       Token Ledger Library
#=================================================
add_library(
        "${PROJECT_NAME}-ledger"
        SHARED
        ${TOKEN_LIB_SOURCES} ${PROTO_HEADER} ${PROTO_SRC} node.grpc.pb.cc node.grpc.pb.h service.grpc.pb.cc service.grpc.pb.h
)
target_link_libraries(
        "${PROJECT_NAME}-ledger"
        ${SQLite3_LIBRARIES} ${Protobuf_LIBRARIES} glog::glog ${LevelDB_LIBRARY}
)

#=================================================
#       Token Ledger Node Exe
#=================================================
add_executable(
        "${PROJECT_NAME}-node"
        main.cc
)
target_link_libraries(
        "${PROJECT_NAME}-node"
        "${PROJECT_NAME}-ledger" ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${Protobuf_LIBRARIES} ${GRPC_GRPC++_LIBRARY} ${GRPC_GRPC++_REFLECTION_LIBRARY} gflags
)

#=================================================
#       Token Ledger RPC Client Exe
#=================================================
add_executable(
        "${PROJECT_NAME}-client"
        client.cc
)
target_link_libraries(
        "${PROJECT_NAME}-client"
        "${PROJECT_NAME}-ledger" ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${Protobuf_LIBRARIES} ${GRPC_GRPC++_LIBRARY} ${GRPC_GRPC++_REFLECTION_LIBRARY} gflags
)
