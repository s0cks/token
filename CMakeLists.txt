cmake_minimum_required(VERSION 3.7)
project("token" CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Libuv REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GTest REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(Threads)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${LIBUV_INCLUDE_DIR})
include_directories(include)
include_directories(src)

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")

file(GLOB_RECURSE TOKEN_LIB_SOURCES include/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_PROTO_SOURCES protos/*.proto)
file(GLOB_RECURSE TOKEN_TEST_SOURCES test/*.cc)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${TOKEN_PROTO_SOURCES})

add_library("${PROJECT_NAME}" STATIC ${TOKEN_LIB_SOURCES} ${PROTO_HEADER} ${PROTO_SRC})
target_link_libraries("${PROJECT_NAME}" ${CRYPTO++_LIBRARIES} ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LIBUV_LIBRARIES})

add_executable("${PROJECT_NAME}-node" main.cc)
target_link_libraries("${PROJECT_NAME}-node" ${PROJECT_NAME})

add_executable("${PROJECT_NAME}-client" client.cc)
target_link_libraries("${PROJECT_NAME}-client" ${PROJECT_NAME})

add_executable("${PROJECT_NAME}-tests" ${TOKEN_TEST_SOURCES})
target_link_libraries("${PROJECT_NAME}-tests" ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME})