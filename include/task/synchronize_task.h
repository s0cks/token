#ifndef TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H
#define TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H

#include "task.h"
#include "block.h"
#include "block_processor.h"

namespace Token{
    class SynchronizeBlockChainTask : public AsyncSessionTask{
    private:
        BlockHeader head_;

        SynchronizeBlockChainTask(uv_loop_t* loop, Session* session, const BlockHeader& head):
            AsyncSessionTask(loop, session),
            head_(head){}

        static inline InventoryItem
        GetItem(const Hash hash){
            return InventoryItem(InventoryItem::kBlock, hash);
        }

        bool ProcessBlock(const BlockPtr& blk){
            BlockHeader header = blk->GetHeader();
            Hash hash = header.GetHash();
            if(!SynchronizeBlockProcessor::Process(blk)){
                LOG(WARNING) << "couldn't process block: " << header;
                return false;
            }
            BlockPool::RemoveBlock(hash);
            BlockChain::Append(blk);
            return true;
        }

        AsyncTaskResult DoWork(){
            std::deque<Hash> work; // we are queuing the blocks just in-case there is an unresolved previous Hash
            work.push_back(head_.GetHash());
            do{
                Hash hash = work.front();
                work.pop_front();

                if(!BlockPool::HasBlock(hash)){
                    LOG(INFO) << "waiting for: " << hash;
                    BlockPool::WaitForBlock(hash);
                }

                BlockPtr blk = BlockPool::GetBlock(hash);
                Hash phash = blk->GetPreviousHash();
                if(!BlockChain::HasBlock(phash)){
                    LOG(WARNING) << "parent block " << phash << " not found, resolving...";
                    work.push_front(hash);
                    work.push_front(phash);
                    continue;
                }

                if(!ProcessBlock(blk)){
                    std::stringstream ss;
                    ss << "couldn't process block: " << hash;
                    return AsyncTaskResult(AsyncTaskResult::kFailed, ss.str());
                }
            } while(!work.empty());
            return AsyncTaskResult(AsyncTaskResult::kSuccessful, "synchronized block chain");
        }
    public:
        ~SynchronizeBlockChainTask() = default;

        DEFINE_ASYNC_TASK(SynchronizeBlockChain);

        InventoryItem GetHead() const{
            return InventoryItem(InventoryItem::kBlock, head_.GetHash());
        }

        static SynchronizeBlockChainTask* NewInstance(uv_loop_t* loop, Session* session, const BlockHeader& head){
            return new SynchronizeBlockChainTask(loop, session, head);
        }
    };
}

#endif //TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H