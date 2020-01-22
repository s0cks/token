#include <glog/logging.h>
#include "block_miner.h"
#include "block_chain.h"

namespace Token{
    void* BlockMiner::MinerThread(void* data){
        char* result;
        BlockQueue* queue = (BlockQueue*)data;
        do{
            Block* blk;
            if(!queue->Pop(&blk)){
                const char* msg = "couldn't pop block from BlockQueue";
                result = strdup(msg);
                pthread_exit(result);
            }

            LOG(WARNING) << "processing block: " << blk->GetHash();
            if(!BlockChain::AppendBlock(blk)){
                const char* msg = "couldn't append new block";
                result = strdup(msg);
                pthread_exit(result);
            }
        } while(true);
    }

    bool BlockMiner::Initialize(){
        BlockMiner* miner = GetInstance();
        return pthread_create(&miner->thread_, NULL, &BlockMiner::MinerThread, GetQueue()) == 0;
    }

    BlockMiner* BlockMiner::GetInstance(){
        static BlockMiner instance;
        return &instance;
    }
}