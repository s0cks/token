cmake_minimum_required(VERSION 3.5)
enable_testing()
project("token" CXX)
set(TOKEN_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)

option(ENABLE_ELASTICSEARCH "Enables Elasticsearch Integration." ON)
option(ENABLE_COMPRESSION "Enables compression." OFF)
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

# Compile w/ compression enabled
if(ENABLE_COMPRESSION)
  message(STATUS "Compiling w/ compression enabled.")
  add_compile_definitions(TOKEN_ENABLE_COMPRESSION)
endif()

# Compile w/ Elasticsearch support
if(ENABLE_ELASTICSEARCH)
  message(STATUS "Compiling w/ Elasticsearch enabled.")
  add_compile_definitions(TOKEN_ENABLE_ELASTICSEARCH)
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
find_package(UUID REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(GTest REQUIRED)
find_package(Libuv REQUIRED)
find_package(HttpParser REQUIRED)
find_package(ZXing REQUIRED)

if(ENABLE_COMPRESSION)
  find_package(ZLIB REQUIRED)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include_directories(/usr/local/include)
endif()

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
  ${CRYPTO++_LIBRARIES} # crypto++
  ${GLOG_LIBRARIES} # glog
  ${LevelDB_LIBRARY} # leveldb
  ${GFLAGS_LIBRARIES} # gflags
  config++ # config++
  ${UUID_LIBRARIES} # uuid
  ${ZXING_LIBRARIES} # zxing
  ${LIBUV_LIBRARIES} # libuv
  ${HTTPPARSER_LIBRARIES} # http-parser
)

if(ENABLE_COMPRESSION)
  message(STATUS "Appending the zlib libraries.")
  list(APPEND TOKEN_LIBRARIES ${ZLIB_LIBRARIES}) # zlib
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Compiling token w/ the following libraries: ${TOKEN_LIBRARIES}")
endif()

# token Ledger Library
add_library("token-ledger" SHARED ${TOKEN_HEADERS} ${TOKEN_SOURCES})
set_target_properties("token-ledger" PROPERTIES VERSION ${TOKEN_VERSION})
set_target_properties("token-ledger" PROPERTIES PUBLIC_HEADER "${TOKEN_HEADERS}")
target_link_libraries("${PROJECT_NAME}-ledger" ${TOKEN_LIBRARIES} png)
target_compile_options(token-ledger PRIVATE -Werror -Wall)

# token Server Executable
add_executable("token-node" main.cc)
target_link_libraries("token-node" "token-ledger" ${TOKEN_LIBRARIES})
target_compile_options(token-node PRIVATE -Werror -Wall)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  # token Unit Tests
  add_executable("token-tests" tests.cc ${TOKEN_TEST_SOURCES})
  target_link_libraries("token-tests" "token-ledger" ${GTEST_BOTH_LIBRARIES} ${TOKEN_LIBRARIES})
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