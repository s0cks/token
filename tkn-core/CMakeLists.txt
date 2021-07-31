project(tkn-core CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Protobuf REQUIRED)
include_directories(protos)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(TKN_CORE_PROTO_SOURCES TKN_CORE_PROTO_HEADERS
    protos/input.proto
    protos/output.proto
    protos/transaction.proto
    protos/block.proto
    protos/unclaimed_transaction.proto
)

add_library(tkn-core STATIC
    ${TKN_CORE_PROTO_HEADERS} ${TKN_CORE_PROTO_SOURCES}
    include/codec/codec.h src/codec/codec.cc
    include/codec/encoder.h
    include/codec/decoder.h

    include/type/product.h src/type/product.cc
    include/type/user.h src/type/user.cc
    include/type/wallet.h

    include/block.h src/block.cc
    include/block_chain.h src/block_chain.cc
    include/block_header.h src/block_header.cc
    include/object.h
    include/proposal.h src/proposal.cc
    include/transaction.h
    include/transaction_indexed.h src/transaction_indexed.cc
    include/transaction_input.h src/transaction_input.cc
    include/transaction_output.h src/transaction_output.cc
    include/transaction_reference.h
    include/transaction_signed.h src/transaction_signed.cc
    include/transaction_unclaimed.h src/transaction_unclaimed.cc
    include/transaction_unsigned.h src/transaction_unsigned.cc
    include/type.h)
target_include_directories(tkn-core PUBLIC ${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tkn-core tkn-utils
    tkn-merkle
    ${CMAKE_THREAD_LIBS_INIT} # pthreads
    ${GFLAGS_LIBRARIES} # gflags
    ${CRYPTOPP_LIBRARIES} # cryptopp
    ${UV_LIBRARIES} # libuv
    ${PNG_LIBRARIES} # libpng
    ${HTTPPARSER_LIBRARIES} # libhttp-parser
    ${LEVELDB_LIBRARIES} # leveldb
    ${ZXING_LIBRARIES} # zxing
    ${UUID_LIBRARIES} # libuuid

    protobuf::libprotobuf
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()