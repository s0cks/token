cmake_minimum_required(VERSION 3.5)
enable_testing()
project("token" CXX)
set(TOKEN_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)
message("Using Modules: ${TOKEN_CMAKE_MODULES}")

# Monkey Patching for Debug Mode
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Compiling Token w/ Debug")
  add_compile_definitions(TOKEN_DEBUG)
else()
  message("Compiling Token w/o Debug")
endif (CMAKE_BUILD_TYPE MATCHES Debug)

# Memory Management Options:
# -------------------------
#   - Default:
#       set(TOKEN_GCMODE Default)
#   - NO-OP:
#       set(TOKEN_GCMODE NoOp)
#   - None:
#       set(TOKEN_GCMODE None)
#---------------------------
if(TOKEN_GCMODE MATCHES None)
  message("Compiling Token w/o a garbage collector")
  add_compile_definitions(TOKEN_GCMODE_NONE)
elseif(TOKEN_GCMODE MATCHES NoOp)
  message("Compiling Token w/ a no-op garbage collector")
  add_compile_definitions(TOKEN_GCMODE_NOOP)
elseif(TOKEN_GCMODE MATCHES Default)
  message("Compiling Token w/ the default garbage collector")
  add_compile_definitions(TOKEN_GCMODE_DEFAULT)
else()
  message("Compiling Token w/ the default garbage collector")
  add_compile_definitions(TOKEN_GCMODE_DEFAULT)
endif(TOKEN_GCMODE MATCHES None)

# Packages
find_package(Threads)
find_package(UUID REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(Libuv REQUIRED)
find_package(GTest REQUIRED)
find_package(HttpParser REQUIRED)
find_package(JsonCpp REQUIRED)

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
  ${CRYPTO++_LIBRARIES} # crypto++
  ${GLOG_LIBRARIES} # glog
  ${LevelDB_LIBRARY} # leveldb
  ${LIBUV_LIBRARIES} # libuv
  ${GFLAGS_LIBRARIES} # gflags
  config++ # config++
  ${UUID_LIBRARIES} # uuid
  ${CMAKE_THREAD_LIBS_INIT} # pthread
  ${HTTPPARSER_LIBRARIES} # http-parser
  ${JsonCpp_LIBRARY} # jsoncpp
)
message("Compiling Token w/ the following libraries: ${TOKEN_LIBRARIES}")

# Token Ledger Library
add_library("token-ledger" SHARED ${TOKEN_HEADERS} ${TOKEN_SOURCES})
set_target_properties("token-ledger" PROPERTIES VERSION ${TOKEN_VERSION})
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

if(CMAKE_BUILD_TYPE MATCHES Debug)
  # Token Unit Tests
  add_executable("token-tests" tests.cc ${TOKEN_TEST_SOURCES})
  target_link_libraries("token-tests" "token-ledger" ${GTEST_BOTH_LIBRARIES} ${TOKEN_LIBRARIES})
  add_test("token-tests" "token-tests")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# should we remove this?
# Install Token Node

install(
  DIRECTORY "include/" # source directory
  DESTINATION "include/token" # target directory
  FILES_MATCHING # install only matched files
  PATTERN "*.h" # select header files
)
install(
  TARGETS token-ledger
  LIBRARY DESTINATION lib
)
install(
  TARGETS token-node
  RUNTIME DESTINATION bin
)