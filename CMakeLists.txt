cmake_minimum_required(VERSION 3.5)
project("token" CXX)

set(TOKEN_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

option(ENABLE_EXPERIMENTAL "Enables experimental features." OFF)

# Monkey Patching for Debug Mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Compiling w/ debug mode.")
    add_compile_definitions(TOKEN_DEBUG)
else ()
    message(STATUS "Compiling w/o debug mode.")
endif ()

# Compile w/ Experimental Features
if (ENABLE_EXPERIMENTAL)
    message(STATUS "Compiling w/ Experimental Features.")
    add_compile_definitions(TOKEN_ENABLE_EXPERIMENTAL)
endif ()

find_package(glog REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(Tests)
add_subdirectory(Sources)

#add_subdirectory(tkn-deeplink)
#add_subdirectory(tkn-http)
#add_subdirectory(tkn-merkle)
#add_subdirectory(tkn-platform)
#add_subdirectory(tkn-rpc)
#add_subdirectory(tkn-runtime)
#add_subdirectory(tkn-task-engine)
#add_subdirectory(tkn-utils)