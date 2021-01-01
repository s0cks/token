#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "pool.h"
#include "block.h"
#include "job/job.h"

namespace Token{
  typedef std::map<User, HashList> UserHashLists;

  class ProcessBlockJob : public WriteBatchJob, BlockVisitor{
    friend class ProcessTransactionJob;
   protected:
    BlockPtr block_;
    std::mutex mutex_; //TODO: remove mutex
    leveldb::WriteBatch *batch_;
    UserHashLists hash_lists_;
    bool clean_; //TODO: remove clean flag?

    JobResult DoWork();
   public:
    ProcessBlockJob(const BlockPtr &blk, bool clean = false):
        WriteBatchJob(nullptr, "ProcessBlock"),
        block_(blk),
        mutex_(),
        batch_(new leveldb::WriteBatch()),
        hash_lists_(),
        clean_(clean){}
    ~ProcessBlockJob(){
      if(batch_)
        delete batch_;
    }

    leveldb::WriteBatch *GetBatch() const{
      return batch_;
    }

    BlockPtr GetBlock() const{
      return block_;
    }

    bool ShouldClean() const{
      return clean_;
    }

    bool Visit(const TransactionPtr &tx);

    void Append(leveldb::WriteBatch *batch){
      std::lock_guard<std::mutex> guard(mutex_);
      batch_->Append((*batch));
    }

    void Track(const UserHashLists &hashes){
      //TODO: optimize
      std::lock_guard<std::mutex> guard(mutex_);
      for(auto &it : hashes){
        auto pos = hash_lists_.find(it.first);
        if(pos == hash_lists_.end()){
          hash_lists_.insert({it.first, it.second});
          continue;
        }

        HashList src = it.second;
        HashList &dst = pos->second;
        dst.insert(src.begin(), src.end());
      }
    }

    leveldb::Status Write(){
      std::lock_guard<std::mutex> guard(mutex_);
      if(!batch_)
        return leveldb::Status::InvalidArgument("batch_ is NULL");
      return ObjectPool::Write(batch_);
    }
  };
}

#endif //TOKEN_PROCESS_BLOCK_H