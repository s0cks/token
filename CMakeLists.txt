cmake_minimum_required(VERSION 3.7)
project("token" CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(GRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(Threads)
find_package(Libuv REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LIBUV_INCLUDE_DIRS})
include_directories(include)
include_directories(src)

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
file(GLOB_RECURSE TOKEN_LIB_SOURCES include/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_PROTO_SOURCES protos/*.proto)
file(GLOB_RECURSE TOKEN_SERVICE_PROTO_SOURCES protos/*.proto)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${TOKEN_PROTO_SOURCES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${CMAKE_BINARY_DIR} ${TOKEN_SERVICE_PROTO_SOURCES})

add_library("${PROJECT_NAME}" STATIC ${TOKEN_LIB_SOURCES} ${PROTO_HEADER} ${PROTO_SRC} ${GRPC_HDRS} ${GRPC_SRCS})
target_link_libraries("${PROJECT_NAME}" ${PROTOBUF_LIBRARY} ${LIBUV_LIBRARIES} cryptopp-static)

add_executable("${PROJECT_NAME}-node" main.cc)
target_link_libraries("${PROJECT_NAME}-node" ${PROJECT_NAME}  ${CMAKE_THREAD_LIBS_INIT} gRPC::grpc++_reflection)

add_executable("${PROJECT_NAME}-client" client.cc)
target_link_libraries("${PROJECT_NAME}-client" ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} gRPC::grpc++_reflection)