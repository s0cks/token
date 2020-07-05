cmake_minimum_required(VERSION 3.5)
enable_testing()
project("token" CXX)

#TODO: Fix for remote building
#TODO: organize build process into simpler steps
#TODO: create deb package
#TODO: define process for creating docker image
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")

if (CMAKE_BUILD_TYPE MATCHES Debug)
        message("Compiling token w/ debug mode and unit tests")
        add_compile_definitions(TOKEN_DEBUG)
        set(TOKEN_BUILD_TESTS True)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

# Compiles w/ health check support:
# set(TOKEN_HEALTHCHECK_SUPPORT True)

# Changes garbage collector algorithm to Cheney's Mark-Copy Algorithm:
# set(TOKEN_CHENEYGC_SUPPORT True)

# Creates a .deb packages:
# set(TOKEN_CREATE_PACKAGE True)

# Packages
find_package(Threads)
find_package(Crypto++ REQUIRED)
find_package(Protobuf REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(Libuv REQUIRED)

if(TOKEN_HEALTHCHECK_SUPPORT)
        find_package(HttpParser REQUIRED)
endif(TOKEN_HEALTHCHECK_SUPPORT)

if(TOKEN_BUILD_TESTS)
        find_package(GTest REQUIRED)
endif(TOKEN_BUILD_TESTS)

# Include Directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(include)
include_directories(src)
include_directories(${CRYPTO++_INCLUDE_DIR})
include_directories(${LIBUV_INCLUDE_DIRS})
if(TOKEN_BUILD_TESTS)
        include_directories(tests)
endif(TOKEN_BUILD_TESTS)

# Source Sets
file(GLOB_RECURSE TOKEN_HEADERS include/*.h)
file(GLOB_RECURSE TOKEN_SOURCES src/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_PROTOS protos/*.proto)
if(TOKEN_BUILD_TESTS)
        file(GLOB_RECURSE TOKEN_TEST_SOURCES tests/*.h tests/*.cc)
endif(TOKEN_BUILD_TESTS)

# Define Libraries
set(TOKEN_LIBRARIES
        ${CMAKE_THREAD_LIBS_INIT} # pthread
        ${CRYPTO++_LIBRARIES} # crypto++
        ${Protobuf_LIBRARIES} # protobuf
        ${GLOG_LIBRARIES} # glog
        ${LevelDB_LIBRARY} # leveldb
        ${LIBUV_LIBRARIES} # libuv
        ${GFLAGS_LIBRARIES} # gflags
        config++ # config++
)

# Optionally add testing libraries
if(TOKEN_BUILD_TESTS)
        list(APPEND TOKEN_LIBRARIES ${GTEST_LIBRARIES})
endif(TOKEN_BUILD_TESTS)

# Optionally add health check libraries
if(TOKEN_HEALTHCHECK_SUPPORT)
        list(APPEND TOKEN_LIBRARIES ${HTTPPARSER_LIBRARIES})
endif(TOKEN_HEALTHCHECK_SUPPORT)

# Compilation Options
#-----------------------------------------------------------------------------------
# Health Check Support
if(TOKEN_HEALTHCHECK_SUPPORT)
        message("Compiling ledger w/ health check support")
        add_compile_definitions(TOKEN_HEALTHCHECK_SUPPORT)
endif(TOKEN_HEALTHCHECK_SUPPORT)

# Mark-Copy Scavenger Support
if(TOKEN_CHENEYGC_SUPPORT)
        message("Compiling ledger w/ mark-copy scavenger support")
        add_compile_definitions(TOKEN_USE_CHENEYGC)
endif(TOKEN_CHENEYGC_SUPPORT)
#------------------------------------------------------------------------------------


# Generate Proto Sources
protobuf_generate_cpp(
        TOKEN_PROTO_SOURCES
        TOKEN_PROTO_HEADERS
        ${TOKEN_PROTOS}
)

message("Compiling Token w/ the following libraries: ${TOKEN_LIBRARIES}")

# Token Ledger Library
add_library("token-ledger" SHARED ${TOKEN_HEADERS} ${TOKEN_SOURCES} ${TOKEN_PROTO_HEADERS} ${TOKEN_PROTO_SOURCES})
target_link_libraries("${PROJECT_NAME}-ledger" ${TOKEN_LIBRARIES})

# Token Node Executable
add_executable("token-node" main.cc ${TOKEN_HEADERS} ${TOKEN_SOURCES} ${TOKEN_PROTO_HEADERS} ${TOKEN_PROTO_SOURCES})
target_link_libraries("token-node" ${TOKEN_LIBRARIES})

# Token Client Executable
add_executable("token-client" client.cc ${TOKEN_HEADERS} ${TOKEN_SOURCES} ${TOKEN_PROTO_HEADERS} ${TOKEN_PROTO_SOURCES})
target_link_libraries("token-client" ${TOKEN_LIBRARIES})

# Token Unit Tests Executable
if(TOKEN_BUILD_TESTS)
        message("Enabling testing")
        enable_testing()

        add_executable("token-tests" tests.cc ${TOKEN_TEST_SOURCES} ${TOKEN_HEADERS} ${TOKEN_SOURCES} ${TOKEN_PROTO_HEADERS} ${TOKEN_PROTO_SOURCES})
        target_link_libraries("token-tests" ${TOKEN_LIBRARIES})
endif(TOKEN_BUILD_TESTS)

# Token Debian Package
if(TOKEN_CREATE_PACKAGE)
        message("Creating a .deb package")
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcrypto++-dev (>= 5.6.4-7), libprotobuf-dev (>= 3.0.0-9), libleveldb-dev (>= 1.18-5), libgoogle-glog-dev (>= 0.3.4-2), libgflags-dev (>= 1.30.1-1), libuv1-dev (>= 1.9.1-3), libhttp-parser-dev (>= 2.1-2)")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tazz")
        include(CPack)
endif(TOKEN_CREATE_PACKAGE)