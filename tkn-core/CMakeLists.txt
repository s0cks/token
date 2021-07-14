project(tkn-core CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-core STATIC
    include/codec/codec.h src/codec/codec.cc
    include/codec/encoder.h
    include/codec/decoder.h
    include/type/product.h src/type/product.cc
    include/type/proposal.h src/type/proposal.cc
    include/type/user.h src/type/user.cc
    include/type/wallet.h
    include/block.h src/block.cc
    include/block_header.h src/block_header.cc
    include/object.h
    include/transaction.h
    include/transaction_indexed.h src/transaction_indexed.cc
    include/transaction_input.h src/transaction_input.cc
    include/transaction_output.h src/transaction_output.cc
    include/transaction_reference.h
    include/transaction_signed.h src/transaction_signed.cc
    include/transaction_unclaimed.h src/transaction_unclaimed.cc
    include/transaction_unsigned.h src/transaction_unsigned.cc
    include/type.h)
target_include_directories(tkn-core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(tkn-core tkn-utils tkn-merkle)