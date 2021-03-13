#ifndef TOKEN_INCLUDE_JOB_SYNCHRONIZE_H
#define TOKEN_INCLUDE_JOB_SYNCHRONIZE_H

#include "pool.h"
#include "job/job.h"
#include "blockchain.h"
#include "rpc/rpc_session.h"
#include "job/processor.h"

namespace token{
  class SynchronizeJob : public Job{
   private:
    RpcSession* session_;
    BlockChain* chain_;
    BlockHeader head_;


    inline BlockChain*
    GetChain() const{
      return chain_;
    }

    bool ProcessBlock(const BlockPtr& blk){
      JobQueue* queue = JobScheduler::GetThreadQueue();
      Hash hash = blk->GetHash();

      ProcessBlockJob* job = new ProcessBlockJob(blk);
      queue->Push(job);
      while(!job->IsFinished()); //spin

      ObjectPool::RemoveBlock(hash);
      GetChain()->Append(blk);
      return true;
    }
   protected:
    JobResult DoWork(){
      std::deque<Hash> work; // we are queuing the blocks just in-case there is an unresolved previous Hash
      work.push_back(head_.GetHash());
      do{
        Hash hash = work.front();
        work.pop_front();

        if(!ObjectPool::HasBlock(hash)){
          LOG(WARNING) << "waiting for: " << hash;
          ObjectPool::WaitForBlock(hash);
        }

        BlockPtr blk = ObjectPool::GetBlock(hash);
        Hash phash = blk->GetPreviousHash();
        if(!GetChain()->HasBlock(phash)){
          LOG(WARNING) << "parent block " << phash << " not found, resolving...";
          work.push_front(hash);
          work.push_front(phash);
          continue;
        }

        if(!ProcessBlock(blk)){
          std::stringstream ss;
          ss << "couldn't process block: " << hash;
          return Failed(ss);
        }
      } while(!work.empty());
      return Success("done.");
    }
   public:
    SynchronizeJob(Job* parent, RpcSession* session, BlockChain* chain, const BlockHeader& head):
      Job(parent, "SynchronizeJob"),
      session_(session),
      chain_(chain),
      head_(head){}
    SynchronizeJob(RpcSession* session, BlockChain* chain, const BlockHeader& head):
      SynchronizeJob(nullptr, session, chain, head){}
    ~SynchronizeJob() = default;
  };
}

#endif //TOKEN_INCLUDE_JOB_SYNCHRONIZE_H