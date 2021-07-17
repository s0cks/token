project(tkn-runtime CXX)
set(CMAKE_CXX_STANDARD 11)

add_executable(tkn-runtime
    include/blockchain.h src/blockchain.cc
    include/configuration.h src/configuration.cc
    include/flags.h src/flags.cc
    include/miner.h src/miner.cc
    include/pool.h src/pool.cc
    include/server.h
    include/wallet_manager.h src/wallet_manager.cc
    src/main.cc)
target_link_libraries(tkn-runtime tkn-platform tkn-utils tkn-core tkn-merkle tkn-task-engine tkn-rpc tkn-deeplink)
target_include_directories(tkn-runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)