#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"

namespace Token{
    //TODO:
    // - establish timer for mining
    // - refactor
    class BlockMiner{
    private:
        static void HandleMineCallback(uv_timer_t* handle);
        static void HandleExitCallback(uv_async_t* handle);
        static void* MinerThread(void* data);

        friend class BlockChain;
    public:
        static const uint64_t kMiningIntervalMilliseconds = 1 * 1000;

        ~BlockMiner(){}

        static bool Initialize();
    };
}

#endif //TOKEN_BLOCK_MINER_H