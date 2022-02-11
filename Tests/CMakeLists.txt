find_package(GTest REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_BINARY_DIR}/Sources/) #TODO: Fixme

add_executable(token-tests
    main.cc
    token/test_input.cc
    token/test_user.cc
    token/test_product.cc
    token/test_output.cc
    token/helpers.h
    token/test_buffer.cc
    token/test_transaction_reference.cc
    token/test_transaction.cc token/test_merkle.cc token/test_hash.cc token/test_block.cc)
target_link_libraries(token-tests
    token
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARIES}
    glog::glog
    GTest::GTest
    ${OPENSSL_LIBRARIES})