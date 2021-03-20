cmake_minimum_required(VERSION 3.5)
enable_testing()
project("token" CXX)
set(TOKEN_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)

option(ENABLE_EXPERIMENTAL "Enables experimental features." ON)
option(ENABLE_SERVER "Enables the block chain server." ON)
option(ENABLE_HEALTH_SERVICE "Enable the health http service." ON)
option(ENABLE_REST_SERVICE "Enable the rest http service." ON)

# Monkey Patching for Debug Mode
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Compiling w/ debug mode.")
  add_compile_definitions(TOKEN_DEBUG)
else()
  message(STATUS "Compiling w/o debug mode.")
endif()

# Compile w/ Experimental Features
if(ENABLE_EXPERIMENTAL)
  message(STATUS "Compiling w/ Experimental Features.")
  add_compile_definitions(TOKEN_ENABLE_EXPERIMENTAL)
endif()

# Compile w/ the rpc if enabled
if(ENABLE_SERVER)
  message(STATUS "Compiling w/ the server enabled.")
  add_compile_definitions(TOKEN_ENABLE_SERVER)
endif()

# Compile w/ the controller service if enabled
if(ENABLE_REST_SERVICE)
  message(STATUS "Compiling w/ the rest http service enabled.")
  add_compile_definitions(TOKEN_ENABLE_REST_SERVICE)
endif()

# Compile w/ the health check service if enabled
if(ENABLE_HEALTH_SERVICE)
  message(STATUS "Compiling w/ the health-check http service enabled.")
  add_compile_definitions(TOKEN_ENABLE_HEALTH_SERVICE)
endif()

# Packages
find_package(Threads)
find_package(uv REQUIRED)
find_package(png REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
find_package(cryptopp REQUIRED)
find_package(httpparser REQUIRED)
find_package(leveldb REQUIRED)
find_package(zxing REQUIRED)
find_package(uuid REQUIRED)
find_package(gmock REQUIRED)
find_package(gtest REQUIRED)

# Include Directories
include_directories(include)
include_directories(src)
include_directories(tests)

# Source Sets
file(GLOB_RECURSE TOKEN_HEADERS include/*.h)
file(GLOB_RECURSE TOKEN_SOURCES src/*.h src/*.cc)
file(GLOB_RECURSE TOKEN_TEST_SOURCES tests/*.h tests/*.cc)

# Define Libraries
set(TOKEN_LIBRARIES
   ${CMAKE_THREAD_LIBS_INIT} # pthreads
   ${GLOG_LIBRARIES} # glog
   ${GFLAGS_LIBRARIES} # gflags
   ${CRYPTOPP_LIBRARIES} # cryptopp
   ${UV_LIBRARIES} # libuv
   ${PNG_LIBRARIES} # libpng
   ${HTTPPARSER_LIBRARIES} # libhttp-parser
   ${LEVELDB_LIBRARIES} # leveldb
   ${ZXING_LIBRARIES} # zxing
   ${UUID_LIBRARIES} # libuuid
)

# token Ledger Library
add_library("token-ledger" STATIC ${TOKEN_HEADERS} ${TOKEN_SOURCES})
set_target_properties("token-ledger" PROPERTIES VERSION ${TOKEN_VERSION})
set_target_properties("token-ledger" PROPERTIES PUBLIC_HEADER "${TOKEN_HEADERS}")
target_link_libraries("${PROJECT_NAME}-ledger" ${TOKEN_LIBRARIES})
target_compile_options(token-ledger PRIVATE -Werror -Wall)

# token Server Executable
add_executable("token-node" main.cc)
target_link_libraries("token-node" "token-ledger" ${TOKEN_LIBRARIES})
target_compile_options(token-node PRIVATE -Werror -Wall)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  # token Unit Tests
  add_executable("token-tests" tests.cc ${TOKEN_TEST_SOURCES})
  target_link_libraries("token-tests" "token-ledger" ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES} ${TOKEN_LIBRARIES})
  add_test("token-tests" "token-tests")
endif()

# should we remove this?
# Install token Node

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