cmake_minimum_required(VERSION 3.7)
project("token" CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(GRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GTest REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(Threads)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(include)
include_directories(src)

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
file(GLOB_RECURSE TOKEN_SOURCES include/*.h src/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_PROTO_SOURCES protos/*.proto)
file(GLOB_RECURSE TOKEN_RPC_SOURCES protos/service.proto)
file(GLOB_RECURSE TOKEN_TEST_SOURCES test/*.h test/*.cc)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${TOKEN_PROTO_SOURCES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${CMAKE_BINARY_DIR} ${TOKEN_RPC_SOURCES})

add_library("${PROJECT_NAME}" STATIC ${TOKEN_SOURCES} ${PROTO_HEADER} ${PROTO_SRC})
target_link_libraries("${PROJECT_NAME}" ${CRYPTO++_LIBRARIES} ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable("${PROJECT_NAME}-tests" ${TOKEN_TEST_SOURCES})
target_link_libraries("${PROJECT_NAME}-tests" ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME})

add_executable("${PROJECT_NAME}-node" main.cc ${GRPC_SRCS} ${GRPC_HDRS})
target_link_libraries("${PROJECT_NAME}-node" ${PROJECT_NAME} gRPC::grpc++_reflection)

add_executable("${PROJECT_NAME}-client" client.cc ${GRPC_SRCS} ${GRPC_HDRS})
target_link_libraries("${PROJECT_NAME}-client" ${PROJECT_NAME} gRPC::grpc++_reflection)