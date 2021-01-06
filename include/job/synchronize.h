#ifndef TOKEN_INCLUDE_JOB_SYNCHRONIZE_H
#define TOKEN_INCLUDE_JOB_SYNCHRONIZE_H

#include "pool.h"
#include "session.h"
#include "job/job.h"
#include "job/processor.h"

namespace Token{
  class SynchronizeJob : public Job{
   private:
    Session* session_;
    BlockHeader head_;

    bool ProcessBlock(const BlockPtr& blk){
      BlockHeader header = blk->GetHeader();
      Hash hash = header.GetHash();

      JobWorker* worker = JobScheduler::GetRandomWorker();
      ProcessBlockJob* job = new ProcessBlockJob(blk);
      worker->Submit(job);
      worker->Wait(job);

      ObjectPool::RemoveBlock(hash);
      BlockChain::Append(blk);
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
        if(!BlockChain::HasBlock(phash)){
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
    SynchronizeJob(Job* parent, Session* session, const BlockHeader& head):
      Job(parent, "Synchronize"),
      session_(session),
      head_(head){}
    SynchronizeJob(Session* session, const BlockHeader& head):
      Job(nullptr, "Synchronize"),
      session_(session),
      head_(head){}
    ~SynchronizeJob() = default;
  };
}

#endif //TOKEN_INCLUDE_JOB_SYNCHRONIZE_H