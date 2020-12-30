#ifndef TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H
#define TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H

#include "task.h"
#include "pool.h"
#include "block.h"

#include "job/scheduler.h"
#include "job/process_block.h"

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

            JobWorker* worker = JobScheduler::GetRandomWorker();
            ProcessBlockJob* job = new ProcessBlockJob(blk);
            worker->Submit(job);
            worker->Wait(job);

            ObjectPool::RemoveObject(hash);
            BlockChain::Append(blk);
            return true;
        }

        AsyncTaskResult DoWork(){
            std::deque<Hash> work; // we are queuing the blocks just in-case there is an unresolved previous Hash
            work.push_back(head_.GetHash());
            do{
                Hash hash = work.front();
                work.pop_front();

                if(!ObjectPool::HasObject(hash)){
                    LOG(WARNING) << "waiting for: " << hash;
                    ObjectPool::WaitForObject(hash);
                }

                BlockPtr blk = ObjectPool::GetBlock(hash);
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

        std::string ToString() const{
            //TODO: refactor
            std::stringstream ss;
            ss << "SynchronizeBlockChainTask()";
            return ss.str();
        }

        static SynchronizeBlockChainTask* NewInstance(uv_loop_t* loop, Session* session, const BlockHeader& head){
            return new SynchronizeBlockChainTask(loop, session, head);
        }
    };
}

#endif //TOKEN_SYNCHRONIZE_BLOCKCHAIN_TASK_H