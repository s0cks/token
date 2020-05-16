#include <glog/logging.h>
#include <algorithm>
#include "block_miner.h"
#include "block_chain.h"
#include "node/server.h"
#include "node/message.h"
#include "block_validator.h"

namespace Token{
    static pthread_t thread;
    static uv_timer_t mine_timer_;
    static uv_async_t exit_handle;

    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        if(TransactionPool::GetSize() >= 2){
            std::vector<Transaction> txs;
            if(!TransactionPool::GetTransactions(txs)){
                LOG(ERROR) << "couldn't get transactions from transaction pool";
                return;
            }
            std::sort(txs.begin(), txs.end());

            BlockHeader head = BlockChain::GetHead();
            Block block(head, { txs });

            uint256_t hash = block.GetHash();
            BlockValidator validator(&block);
            if(!validator.IsValid()){
                LOG(ERROR) << "the following block contains " << validator.GetNumberOfInvalidTransactions() << " invalid transactions: " << hash;

                for(auto it = validator.invalid_begin();
                    it != validator.invalid_end();
                    it++){
                    uint256_t invalid_tx = it->GetHash();
                    if(!TransactionPool::RemoveTransaction(invalid_tx)){
                        LOG(ERROR) << "couldn't remove invalid transaction: " << invalid_tx;
                        continue;
                    }

                    LOG(WARNING) << "removed invalid transaction: " << invalid_tx;
                }
                return;
            }

            if(!BlockChain::GetInstance()->Append(&block)){
                LOG(ERROR) << "couldn't append new block: " << hash;
                return;
            }

            for(auto it = validator.valid_begin();
                it != validator.valid_end();
                it++){
                if(!TransactionPool::RemoveTransaction(it->GetHash())){
                    LOG(ERROR) << "couldn't remove transaction: " << it->GetHash();
                    continue;
                }
            }

            //TODO: BlockChainServer::BroadcastInventory(block);
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