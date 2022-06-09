cmake_minimum_required(VERSION 3.5)
project("token" CXX)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

file(GLOB_RECURSE TOKEN_CMAKE_MODULES ${CMAKE_MODULE_PATH}/*.cmake)

find_package(Glog REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Monkey Patching for Debug Mode
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "[TOKEN] Compiling w/ debug mode.")
  add_compile_definitions(TOKEN_DEBUG)
endif()

add_subdirectory(Sources)
add_subdirectory(Tests)