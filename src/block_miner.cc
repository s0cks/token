#include <glog/logging.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "block_miner.h"
#include "block_chain.h"
#include "node/node.h"

namespace Token{
    static uv_async_t exit_handle;
    static uv_async_t mine_handle;
    static pthread_t thread;
    static std::deque<Block*> work;
    static std::mutex mutex;
    static std::condition_variable cv;

    bool BlockMiner::ScheduleBlock(Block* block){
        LOG(INFO) << "scheduling block: " << block->GetHash();

        std::unique_lock<std::mutex> ulock(mutex);
        work.push_back(block);
        ulock.unlock();
        cv.notify_one();

        uv_async_send(&mine_handle);
        return true;
    }

    void BlockMiner::HandleMineCallback(uv_async_t* handle){
        std::unique_lock<std::mutex> ulock(mutex);
        while(work.empty()) cv.wait(ulock);
        Block* block = work.front();
        work.pop_front();
        if(block == nullptr){
            LOG(WARNING) << "couldn't pop block from work queue";
            return;
        }
        LOG(INFO) << "mining block: " << block->GetHash();
        BlockChain* chain = BlockChain::GetInstance();
        if(!chain->Append(block)){
            LOG(WARNING) << "couldn't append block: " << block->GetHash();
            return;
        }
        BlockChainServer::BroadcastBlockToPeers(block);
    }

    void BlockMiner::HandleExitCallback(uv_async_t* handle){
        LOG(INFO) << "stopping miner thread...";
        if(uv_is_closing((uv_handle_t*)handle) == 0){
            uv_close((uv_handle_t*)handle, NULL);
        }
        pthread_exit(nullptr);
    }

    void* BlockMiner::MinerThread(void* data){
        LOG(INFO) << "starting block miner thread...";
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &exit_handle, &HandleExitCallback);
        uv_async_init(loop, &mine_handle, &HandleMineCallback);
        LOG(INFO) << "running event loop...";
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockMiner::StartMinerThread(){
        return pthread_create(&thread, NULL, &BlockMiner::MinerThread, nullptr) == 0;
    }
}