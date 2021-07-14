project(tkn-runtime CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-runtime STATIC
    include/configuration.h src/configuration.cc
    include/env.h
    include/flags.h src/flags.cc
    include/inspector.h
    include/miner.h src/miner.cc
    include/pool.h src/pool.cc
    include/wallet_manager.h src/wallet_manager.cc
    src/block_builder.h
    src/blockchain_initializer.h src/blockchain_initializer.cc
    src/blockchain_pruner.h)
target_include_directories(tkn-runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)