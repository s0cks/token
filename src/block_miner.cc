#include <glog/logging.h>
#include "block_miner.h"
#include "block_chain.h"
#include "node/node.h"

namespace Token{
    static pthread_t thread;
    static uv_timer_t mine_timer_;
    static uv_async_t exit_handle;

    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        if(TransactionPool::GetSize() >= 1){
            std::vector<Transaction> txs;
            if(!TransactionPool::GetTransactions(txs)){
                LOG(ERROR) << "couldn't get transactions from transaction pool";
                return;
            }

            BlockHeader head = BlockChain::GetHead();
            Block nblock(head, { txs });
            if(!BlockChain::GetInstance()->Append(&nblock)){
                LOG(ERROR) << "couldn't append new block: " << nblock.GetHash();
                return;
            }

            for(auto& it : txs){
                if(!TransactionPool::RemoveTransaction(it.GetHash())){
                    LOG(ERROR) << "couldn't remove transaction from pool: " << it.GetHash();
                    return;
                }
            }
        }
    }

    void BlockMiner::HandleExitCallback(uv_async_t* handle){
        if(uv_is_closing((uv_handle_t*)handle) == 0){
            uv_close((uv_handle_t*)handle, NULL);
        }
        pthread_exit(nullptr);
    }

    void* BlockMiner::MinerThread(void* data){
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &exit_handle, &HandleExitCallback);
        uv_timer_init(loop, &mine_timer_);
        uv_timer_start(&mine_timer_, &HandleMineCallback, 0, BlockMiner::kMiningIntervalMilliseconds);
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockMiner::Initialize(){
        return pthread_create(&thread, NULL, &BlockMiner::MinerThread, nullptr) == 0;
    }
}