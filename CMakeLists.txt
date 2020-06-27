cmake_minimum_required(VERSION 3.5)
        enable_testing()
project("token" CXX)

#TODO: Fix for remote building
#TODO: organize build process into simpler steps
#TODO: create deb package
#TODO: define process for creating docker image
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)
        message("Compiling token w/ debug mode")
        add_compile_definitions(TOKEN_ENABLE_DEBUG)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

# Changes garbage collector algorithm to Cheney's Mark-Copy Algorithm
# add_compile_definitions(TOKEN_USE_CHENEYGC)

find_package(Protobuf REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(Threads)
find_package(Glog REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Gflags REQUIRED)
find_package(Libuv REQUIRED)
find_package(HttpParser REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CRYPTO++_INCLUDE_DIR})
include_directories(${LIBUV_INCLUDE_DIRS})
include_directories(${HTTPPARSER_INCLUDE_DIR})
include_directories(include)
include_directories(src)

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
file(GLOB TOKEN_LIB_SOURCES include/*.h src/*.cc src/*.h src/alloc/*.h src/alloc/*.cc)
file(GLOB TOKEN_LIB_NET_SOURCES include/node/*.h src/node/*.h src/node/*.cc)
file(GLOB_RECURSE TOKEN_PROTO_SOURCES protos/*.proto)
#file(GLOB_RECURSE TOKEN_TEST_SOURCES tests/*.h tests/*.cc)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${TOKEN_PROTO_SOURCES})

#=================================================
#       Token Ledger Library
#=================================================
add_library(
        "${PROJECT_NAME}-ledger"
        SHARED
        ${TOKEN_LIB_SOURCES} ${TOKEN_LIB_NET_SOURCES} ${PROTO_HEADER} ${PROTO_SRC}
)
target_link_libraries(
        "${PROJECT_NAME}-ledger"
       ${HTTPPARSER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${Protobuf_LIBRARIES} ${GLOG_LIBRARIES} ${LevelDB_LIBRARY} ${LIBUV_LIBRARIES}
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
        "${PROJECT_NAME}-ledger" ${HTTPPARSER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES}
)

#=================================================
#       Token Ledger Client Exe
#=================================================
add_executable(
        "${PROJECT_NAME}-client"
        client.cc
)
target_link_libraries(
        "${PROJECT_NAME}-client"
        "${PROJECT_NAME}-ledger" ${HTTPPARSER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES}
)

#=================================================
#       Token Ledger Unit Tests Exe
#=================================================
#add_executable(
#        "${PROJECT_NAME}-tests"
#        tests.cc ${TOKEN_TEST_SOURCES}
#)
#target_link_libraries(
#        "${PROJECT_NAME}-tests"
#        "${PROJECT_NAME}-ledger" ${CMAKE_THREAD_LIBS_INIT} ${CRYPTO++_LIBRARIES} ${Protobuf_LIBRARIES} gRPC::grpc++ gRPC::grpc++_reflection ${GFLAGS_LIBRARIES} ${GTEST_BOTH_LIBRARIES}
#)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcrypto++-dev (>= 5.6.4-7), libprotobuf-dev (>= 3.0.0-9), libleveldb-dev (>= 1.18-5), libgoogle-glog-dev (>= 0.3.4-2), libgflags-dev (>= 1.30.1-1), libuv1-dev (>= 1.9.1-3), libhttp-parser-dev (>= 2.1-2)")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tazz")
include(CPack)