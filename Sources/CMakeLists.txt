configure_file(token/token.in.h ${PROJECT_BINARY_DIR}/token/token.h)

include_directories(${PROJECT_BINARY_DIR})

add_library(token
    ${PROJECT_BINARY_DIR}/token/token.h token/token.cc
    token/platform.h
    token/bignumber.h
    token/uint256.h
    token/sha256.h token/sha256.cc)
target_link_libraries(token
    ${CMAKE_THREAD_LIBS_INIT}
    glog::glog
    ${GFLAGS_LIBRARIES}
    OpenSSL::Crypto)
target_include_directories(token PUBLIC ${PROJECT_SOURCE_DIR}/Sources ${PROJECT_BINARY_DIR})