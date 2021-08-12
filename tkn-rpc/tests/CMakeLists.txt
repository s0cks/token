find_package(GTest REQUIRED)
find_package(Threads)
find_package(uv REQUIRED)
find_package(png REQUIRED)
find_package(gflags REQUIRED)
find_package(cryptopp REQUIRED)
find_package(httpparser REQUIRED)
find_package(leveldb REQUIRED)
find_package(zxing REQUIRED)
find_package(uuid REQUIRED)

add_executable(tkn-tests-rpc
    test_rpc_message_version.h test_rpc_message_version.cc
    test_rpc_message_verack.h test_rpc_message_verack.cc)
target_link_libraries(tkn-tests-rpc
    tkn-rpc
    GTest::GTest
    GTest::Main
    ${CMAKE_THREAD_LIBS_INIT} # pthreads
    ${GFLAGS_LIBRARIES} # gflags
    ${CRYPTOPP_LIBRARIES} # cryptopp
    ${UV_LIBRARIES} # libuv
    ${PNG_LIBRARIES} # libpng
    ${HTTPPARSER_LIBRARIES} # libhttp-parser
    ${LEVELDB_LIBRARIES} # leveldb
    ${ZXING_LIBRARIES} # zxing
    ${UUID_LIBRARIES} # libuuid
)
add_test(VersionMessageTest VerackMessageTest tkn-tests-rpc)