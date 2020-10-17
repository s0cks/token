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

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)
message("Using Modules: ${TOKEN_CMAKE_MODULES}")

# Monkey Patching for Debug Mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
        message("Compiling token w/ debug mode")
        add_compile_definitions(TOKEN_DEBUG)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

# Compiles w/ health check support:
# set(TOKEN_HEALTHCHECK_SUPPORT True)

# Changes garbage collector algorithm to Cheney's Mark-Copy Algorithm:
# set(TOKEN_USE_KOA True)

# Creates a .deb packages:
# set(TOKEN_CREATE_PACKAGE True)

# Packages
find_package(Threads)
find_package(UUID REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(Libuv REQUIRED)
find_package(GTest REQUIRED)

if(TOKEN_HEALTHCHECK_SUPPORT)
        find_package(HttpParser REQUIRED)
endif(TOKEN_HEALTHCHECK_SUPPORT)

# Include Directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(include)
include_directories(src)
include_directories(tests)

# Source Sets
file(GLOB_RECURSE TOKEN_HEADERS include/*.h)
file(GLOB_RECURSE TOKEN_SOURCES src/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_PROTOS protos/*.proto)
file(GLOB_RECURSE TOKEN_TEST_SOURCES tests/*.h tests/*.cc)

# Define Libraries
set(TOKEN_LIBRARIES
        ${GTEST_BOTH_LIBRARIES} # gtest
        ${CRYPTO++_LIBRARIES} # crypto++
        ${GLOG_LIBRARIES} # glog
        ${LevelDB_LIBRARY} # leveldb
        ${LIBUV_LIBRARIES} # libuv
        ${GFLAGS_LIBRARIES} # gflags
        config++ # config++
        ${UUID_LIBRARIES} # uuid
        ${CMAKE_THREAD_LIBS_INIT} # pthread
)

# Optionally add health check libraries
if(TOKEN_HEALTHCHECK_SUPPORT)
        list(APPEND TOKEN_LIBRARIES ${HTTPPARSER_LIBRARIES})
endif(TOKEN_HEALTHCHECK_SUPPORT)

message("Compiling Token w/ the following libraries: ${TOKEN_LIBRARIES}")

# Token Ledger Library
add_library("token-ledger" SHARED ${TOKEN_HEADERS} ${TOKEN_SOURCES})
set_target_properties("token-ledger" PROPERTIES PUBLIC_HEADER "${TOKEN_HEADERS}")
target_link_libraries("${PROJECT_NAME}-ledger" ${TOKEN_LIBRARIES})
target_compile_options(token-ledger PRIVATE -Werror -Wall -Wextra)

# Token Server Executable
add_executable("token-node" main.cc)
target_link_libraries("token-node" "token-ledger" ${TOKEN_LIBRARIES})

# Token Client Executable
add_executable("token-client" client.cc)
target_link_libraries("token-client" "token-ledger" ${TOKEN_LIBRARIES})

# Token Inspect Client Executable
add_executable("token-inspector" inspector.cc)
target_link_libraries("token-inspector" "token-ledger" ${TOKEN_LIBRARIES})

# Token Unit Tests
#add_executable("token-tests" tests.cc ${TOKEN_TEST_SOURCES})
#target_link_libraries("token-tests" "token-ledger" ${TOKEN_LIBRARIES})
#add_test("token-tests" "token-tests")

# should we remove this?
# Install Token Node
install(
        TARGETS token-ledger
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/token
)
install(
        TARGETS token-node
        RUNTIME DESTINATION bin
)