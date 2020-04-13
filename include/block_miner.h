#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"
#include "block.h"

namespace Token{
    class BlockMiner{
    private:
        static void HandleExitCallback(uv_async_t* handle);
        static void HandleMineCallback(uv_async_t* handle);
        static void* MinerThread(void* data);

        friend class BlockChain;
    public:
        ~BlockMiner(){}

        static bool StartMinerThread();
        static bool ScheduleBlock(Block* block);
        static bool ScheduleRawBlock(const Proto::BlockChain::Block& raw){
            return ScheduleBlock(new Block(raw));
        }
    };
}

#endif //TOKEN_BLOCK_MINER_H