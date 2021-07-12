cmake_minimum_required(VERSION 3.5)
project("token" CXX)
set(TOKEN_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)

option(ENABLE_EXPERIMENTAL "Enables experimental features." ON)

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
find_package(gtest REQUIRED)
find_package(gmock REQUIRED)

# Define Libraries
set(TKN_LIBRARIES
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

include_directories(include)
include_directories(tests)

file(GLOB_RECURSE TKN_HEADERS include/*.h)
file(GLOB_RECURSE TKN_SOURCES src/*.h src/*.cc)
file(GLOB_RECURSE TKN_TEST_SOURCES tests/*.h tests/*.cc)

add_subdirectory(tkn-platform)
add_subdirectory(tkn-utils)

add_library(tkn-ledger STATIC ${TKN_HEADERS} ${TKN_SOURCES})
target_include_directories(tkn-ledger PUBLIC include)
target_link_libraries(tkn-ledger ${TKN_LIBRARIES} tkn-platform tkn-utils)

add_executable(tkn-node main.cc)
target_link_libraries(tkn-node tkn-ledger ${TKN_LIBRARIES})

add_executable(tkn-tests tests.cc ${TKN_TEST_SOURCES})
target_link_libraries(tkn-tests tkn-ledger ${TKN_LIBRARIES} ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})