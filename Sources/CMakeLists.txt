configure_file(token.h.in ${PROJECT_BINARY_DIR}/token.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(token SHARED
    ${PROJECT_BINARY_DIR}/token.h token.cc
    token/platform.h
    token/crypto.h
    token/hash.h
    token/timestamp.h
    token/address.h token/address.cc
    token/transaction_reference.h
    token/buffer.h
    token/type.h
    token/object.h
    token/user.h
    token/product.h
    token/input.h
    token/output.h
    token/transaction.h
    token/hash.cc
    token/merkle.h token/merkle.cc
    token/block.h token/block.cc)
target_link_libraries(token
    ${CMAKE_THREAD_LIBS_INIT}
    glog::glog
    ${GFLAGS_LIBRARIES}
    ${CRYPTOPP_LIBRARIES})
target_include_directories(token PUBLIC ${PROJECT_SOURCE_DIR}/Sources ${PROJECT_BINARY_DIR})