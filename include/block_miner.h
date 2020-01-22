#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include "common.h"
#include "block_queue.h"

namespace Token{
    class BlockMiner{
    private:
        BlockQueue queue_;
        pthread_t thread_;

        static bool Initialize();
        static void* MinerThread(void* data);
        static BlockMiner* GetInstance();
        static BlockQueue* GetQueue(){
            return &GetInstance()->queue_;
        }

        BlockMiner():
            queue_(10),
            thread_(){}

        friend class BlockChain;
    public:
        ~BlockMiner(){}

        static bool ScheduleBlock(Block* block){
            return GetQueue()->Push(block);
        }

        static bool ScheduleRawBlock(const Messages::Block& raw){
            //Block block(raw);
            Block block(0, {});
            return ScheduleBlock(&block);
        }
    };
}

#endif //TOKEN_BLOCK_MINER_H