configure_file(token/token.in.h ${PROJECT_BINARY_DIR}/token/token.h)

include_directories(${PROJECT_BINARY_DIR})

add_library(token
    ${PROJECT_BINARY_DIR}/token/token.h token/token.cc)
target_link_libraries(token
    ${CMAKE_THREAD_LIBS_INIT}
    glog::glog
    ${GFLAGS_LIBRARIES})
target_include_directories(token PUBLIC ${PROJECT_SOURCE_DIR}/Sources ${PROJECT_BINARY_DIR})