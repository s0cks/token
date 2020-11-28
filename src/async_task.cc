#include "async_task.h"
#include "block_processor.h"
#include "proposal.h"

#include "block_pool.h"

namespace Token{
    static std::mutex mutex_;
    static std::condition_variable cond_;
    static Thread::State state_ = Thread::kStopped;
    static uv_loop_t* loop_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    Thread::State AsyncTaskThread::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void AsyncTaskThread::SetState(Thread::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void AsyncTaskThread::SetLoop(uv_loop_t* loop){
        LOCK_GUARD;
        if(loop_) {
            LOG(INFO) << "destroying async task thread loop....";
            uv_loop_close(loop_);
            uv_loop_delete(loop_);
        }
        loop_ = loop;
    }

    void AsyncTaskThread::WaitForState(Thread::State state){
        LOCK;
        while(GetState() != state) WAIT; // bad?
    }

    void AsyncTaskThread::HandleThread(uword parameter){
        LOG(INFO) << "starting async task thread....";
        uv_loop_t* loop = uv_loop_new();
        SetState(Thread::kRunning);
        SetLoop(loop);

        // do more?
        while(IsRunning()){
            if(IsPaused()){
                LOG(WARNING) << "pausing the async task thread....";
                WaitForState(Thread::kRunning);
            }

            uv_run(loop, UV_RUN_DEFAULT);
        }

        SetLoop(nullptr);
        SetState(Thread::kStopped);
    }

    uv_loop_t* AsyncTaskThread::GetLoop(){
        if(!IsRunning()) return nullptr;
        LOCK_GUARD;
        return loop_;
    }

    bool SynchronizeBlockChainTask::ProcessBlock(const Handle<Block>& block){
        BlockHeader header = block->GetHeader();
        Hash hash = header.GetHash();
        SynchronizeBlockProcessor processor;
        if(!block->Accept(&processor)){
            LOG(WARNING) << "couldn't process block: " << header;
            return false;
        }
        BlockPool::RemoveBlock(hash);
        BlockChain::Append(block);
        return true;
    }


    Result SynchronizeBlockChainTask::DoWork(){
        std::deque<Hash> work; // we are queuing the blocks just in-case there is an unresolved previous Hash
        work.push_back(head_.GetHash());
        do{
            Hash hash = work.front();
            work.pop_front();

            if(!BlockPool::HasBlock(hash)){
                LOG(INFO) << "waiting for: " << hash;
                BlockPool::WaitForBlock(hash);
            }

            Handle<Block> blk = BlockPool::GetBlock(hash);
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
                return Result(Result::kFailed, ss.str());
            }
        } while(!work.empty());
        return Result(Result::kSuccessful, "synchronized block chain");
    }
}